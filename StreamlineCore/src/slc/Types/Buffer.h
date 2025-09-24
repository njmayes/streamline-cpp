#pragma once
#include "slc/Common/Base.h"
#include <cstring>
#include <memory>
#include <stdexcept>
#include <span>
#include <type_traits>
#include <utility>

namespace slc {

	class BufferView;

	class Buffer
	{
	public:
		Buffer() = default;
		explicit Buffer( std::size_t size );
		Buffer( const void* data, std::size_t size );

		Buffer( const Buffer& other );
		Buffer( Buffer&& other ) noexcept;

		Buffer& operator=( const Buffer& other );
		Buffer& operator=( Buffer&& other ) noexcept;

		virtual ~Buffer() = default;

		static Buffer Copy( const void* data, std::size_t size );

		// Access / read
		template < IsStandard T >
		T* As( std::size_t offset = 0 )
		{
			CheckBounds< T >( offset );
			return reinterpret_cast< T* >( Data( offset ) );
		}

		template < IsStandard T >
		const T* As( std::size_t offset = 0 ) const
		{
			CheckBounds< T >( offset );
			return reinterpret_cast< const T* >( Data( offset ) );
		}

		template < IsStandard T >
		T& Read( std::size_t offset = 0 )
		{
			return *As< T >( offset );
		}

		template < IsStandard T >
		T const& Read( std::size_t offset = 0 ) const
		{
			return *As< T >( offset );
		}

		template < IsStandard T >
		void Set( const T& data, std::size_t offset = 0 )
		{
			std::size_t aligned_offset = AlignOffset( offset, alignof( T ) );
			EnsureCapacity( aligned_offset + sizeof( T ) );

			std::memcpy( mData.get() + aligned_offset, &data, sizeof( T ) );
			if ( aligned_offset + sizeof( T ) > mSize )
				mSize = aligned_offset + sizeof( T );
		}

		template < typename T, typename... Args >
		T* Construct( std::size_t offset, Args&&... args )
		{
			std::size_t aligned_offset = AlignOffset( offset, alignof( T ) );
			EnsureCapacity( aligned_offset + sizeof( T ) );

			auto ptr = std::construct_at( reinterpret_cast< T* >( mData + aligned_offset ), std::forward< Args >( args )... );
			if ( aligned_offset + sizeof( T ) > mSize )
				mSize = aligned_offset + sizeof( T );
			return ptr;
		}

		template < typename T >
		void Destroy( std::size_t offset )
		{
			auto* ptr = As< T >( offset );
			std::destroy_at( ptr );
		}

		template < IsStandard T >
		void Push( const T& data )
		{
			std::size_t aligned_offset = AlignOffset( mSize, alignof( T ) );
			Set( data, aligned_offset );
		}

		template < IsStandard T >
		T Pop()
		{
			if ( mSize < sizeof( T ) )
				throw std::runtime_error( "Buffer underflow" );

			std::size_t aligned_offset = AlignOffset( mSize - sizeof( T ), alignof( T ) );
			CheckBounds< T >( aligned_offset );

			T data = Read< T >( aligned_offset );
			mSize = aligned_offset;

			return data;
		}

		Byte* Data( std::size_t offset = 0 )
		{
			if ( offset > mSize )
				throw std::runtime_error( "Data access out of bounds." );
			return Data( offset );
		}

		const Byte* Data( std::size_t offset = 0 ) const
		{
			if ( offset > mSize )
				throw std::runtime_error( "Data access out of bounds." );
			return Data( offset );
		}

		std::size_t Size() const
		{
			return mSize;
		}
		std::size_t Capacity() const
		{
			return mCapacity;
		}

		void Reserve( std::size_t new_capacity );
		void Resize( std::size_t new_size );

		Buffer CopyBytes( std::size_t size, std::size_t offset = 0 ) const;

		template < typename range_t >
			requires std::ranges::contiguous_range< range_t > && IsStandard< std::ranges::range_value_t< range_t > >
		void Append( range_t&& r )
		{
			auto span = std::span{ r };
			auto bytes = std::as_bytes( span );

			EnsureCapacity( mSize + bytes.size() );
			std::memcpy( mData.get() + mSize, bytes.data(), bytes.size() );
			mSize += bytes.size();
		}

		// operator[]
		Byte& operator[]( std::size_t index )
		{
			if ( index >= mSize )
				throw std::runtime_error( "Index out of bounds" );
			return mData[ index ];
		}

		Byte operator[]( std::size_t index ) const
		{
			if ( index >= mSize )
				throw std::runtime_error( "Index out of bounds" );
			return mData[ index ];
		}

		explicit operator bool() const
		{
			return mData != nullptr && mSize > 0;
		}

		// Non-owning view
		BufferView View( std::size_t offset = 0, std::size_t size = Limits< std::size_t >::Max );

	private:
		void EnsureCapacity( std::size_t required )
		{
			if ( required > mCapacity )
				Reserve( required );
		}

		static std::size_t AlignOffset( std::size_t offset, std::size_t alignment )
		{
			return ( offset + alignment - 1 ) & ~( alignment - 1 );
		}

		template < typename T >
		void CheckBounds( std::size_t offset ) const
		{
			if ( offset + sizeof( T ) > mSize )
				throw std::runtime_error( "Buffer access out of bounds or misaligned" );
			if ( reinterpret_cast< uintptr_t >( Data( offset ) ) % alignof( T ) != 0 )
				throw std::runtime_error( "Buffer access misaligned" );
		}

	private:
		std::unique_ptr< Byte[] > mData;
		std::size_t mSize{ 0 };
		std::size_t mCapacity{ 0 };
	};

	///////////////////////////////////////////////////////////////////////////////
	// BufferView: Non-owning lightweight view of a buffer
	///////////////////////////////////////////////////////////////////////////////

	class BufferView
	{
	public:
		BufferView() = default;
		BufferView( void* data, std::size_t size )
			: mData( static_cast< Byte* >( data ), size )
		{}

		BufferView( BufferView const& ) = default;
		BufferView( BufferView&& ) = default;

		BufferView& operator=( BufferView const& ) = default;
		BufferView& operator=( BufferView&& ) = default;

		virtual ~BufferView() = default;

		BufferView( Buffer& buffer, std::size_t offset = 0, std::size_t size = Limits< std::size_t >::Max )
		{
			if ( offset > buffer.Size() )
				throw std::runtime_error( "View offset out of bounds" );

			auto data = buffer.Data( offset );
			auto actual_size = std::min( size, buffer.Size() - offset );
			mData = std::span{ data, actual_size };
		}

		Byte* Data( std::size_t offset = 0 )
		{
			if ( offset > mData.size() )
				throw std::runtime_error( "Data access out of bounds." );
			return mData.data() + offset;
		}

		const Byte* Data( std::size_t offset = 0 ) const
		{
			if ( offset > mData.size() )
				throw std::runtime_error( "Data access out of bounds." );
			return mData.data() + offset;
		}

		std::size_t Size() const
		{
			return mData.size();
		}

		template < IsStandard T >
		T* As( std::size_t offset = 0 )
		{
			if ( offset + sizeof( T ) > mData.size() )
				throw std::runtime_error( "BufferView access out of bounds" );
			if ( reinterpret_cast< uintptr_t >( Data( offset ) ) % alignof( T ) != 0 )
				throw std::runtime_error( "BufferView access misaligned" );
			return reinterpret_cast< T* >( Data( offset ) );
		}

		template < IsStandard T >
		const T* As( std::size_t offset = 0 ) const
		{
			if ( offset + sizeof( T ) > mData.size() )
				throw std::runtime_error( "BufferView access out of bounds" );
			if ( reinterpret_cast< uintptr_t >( Data( offset ) ) % alignof( T ) != 0 )
				throw std::runtime_error( "BufferView access misaligned" );
			return reinterpret_cast< const T* >( Data( offset ) );
		}

		template < IsStandard T >
		T& Read( std::size_t offset = 0 )
		{
			return *As< T >( offset );
		}

		template < IsStandard T >
		T const& Read( std::size_t offset = 0 ) const
		{
			return *As< T >( offset );
		}

		BufferView View( std::size_t offset = 0, std::size_t size = Limits< std::size_t >::Max )
		{
			auto actual_size = std::min( size, Size() - offset );
			return BufferView( Data( offset ), actual_size );
		}

		explicit operator bool() const
		{
			return mData.data() and mData.size();
		}

	private:
		std::span< Byte > mData;
	};

} // namespace slc

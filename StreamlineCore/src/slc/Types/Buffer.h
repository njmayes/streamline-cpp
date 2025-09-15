#pragma once

#include "slc/Common/Base.h"

namespace slc {

	// N.B. this class technically invokes undefined behaviour to read types out of the buffer, but this is generally safe for standard layout types on major compilers.
	class Buffer
	{
	public:
		Buffer() = default;
		Buffer( void* data, size_t size, bool owned = true );
		Buffer( std::size_t size );

		Buffer( Buffer const& );
		Buffer( Buffer&& ) noexcept;

		Buffer& operator=( Buffer const& );
		Buffer& operator=( Buffer&& ) noexcept;

		virtual ~Buffer();

		static Buffer Copy( const void* data, size_t size );

	public:
		template < IsStandard T, typename Self >
		decltype( auto ) As( this Self&& self )
		{
			using ReturnType = std::conditional_t< std::is_const_v< Self >, const T*, T* >;
			return reinterpret_cast< ReturnType >( std::forward< Self >( self ).Data() );
		}

		template < IsStandard T, typename Self >
		decltype( auto ) Read( this Self&& self, std::size_t offset )
		{
			using ReturnType = std::conditional_t< std::is_const_v< Self >, const T&, T& >;
			return *reinterpret_cast< ReturnType >( std::forward< Self >( self ).Data( offset ) );
		}

		template < IsStandard T >
		void Set( const T& data, size_t offset = 0 )
		{
			auto span = std::span< const T, 1 >{ std::addressof( data ), 1 };
			auto bytes = std::as_bytes( span );

			if ( offset + bytes.size() > mSize )
				Resize( offset + bytes.size() );

			std::memcpy( mData + offset, bytes.data(), bytes.size() );
		}

		template < IsStandard T >
		void Push( const T& data )
		{
			Set( data, mSize );
		}

		template < IsStandard T >
		T Pop()
		{
			constexpr std::size_t DataSize = sizeof( T );
			auto data = std::move( Read< T >( mSize - DataSize ) );
			Resize( mSize - DataSize );
			return data;
		}

		Byte* Data( size_t offset = 0 )
		{
			ASSERT( offset < mSize );
			return mData + offset;
		}
		const Byte* Data( size_t offset = 0 ) const
		{
			ASSERT( offset < mSize );
			return mData + offset;
		}

		Buffer View( size_t offset = 0 )
		{
			return Buffer( mData + offset, mSize - offset, false );
		}

		size_t Size() const
		{
			return mSize;
		}

		void Reserve( size_t new_size );
		void Resize( size_t new_size );

		Buffer CopyBytes( size_t size, size_t offset = 0 );

		template < typename range_t >
			requires std::ranges::contiguous_range< range_t > and IsStandard< std::ranges::range_value_t< range_t > >
		void Append( range_t&& r )
		{
			auto span = std::span{ r };
			auto bytes = std::as_bytes( span );
			if ( bytes.size() > mSize )
				Reserve( bytes.size() );

			std::memcpy( mData, bytes.data(), bytes.size() );
			mSize += bytes.size();
		}

	public:
		operator bool() const
		{
			return mData != nullptr and mSize > 0;
		}

		Byte& operator[]( size_t index )
		{
			return mData[ index ];
		}
		Byte operator[]( size_t index ) const
		{
			return mData[ index ];
		}

	private:
		Byte* mData{};
		std::size_t mSize{};
		std::size_t mCapacity{};

		bool mOwned{};
	};
} // namespace slc
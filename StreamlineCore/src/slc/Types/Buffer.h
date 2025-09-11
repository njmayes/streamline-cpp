#pragma once

#include "slc/Common/Base.h"

namespace slc {

	// N.B. this class technically invokes undefined behaviour to read types out of the buffer, but this is generally safe for standard layout types on major compilers.
	class Buffer
	{
	public:
		Buffer() = default;
		Buffer( std::nullptr_t )
		{}
		Buffer( size_t size )
		{
			mData.reserve( size );
		}

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

			if ( offset + bytes.size() > mData.size() )
				Resize( offset + bytes.size() );

			std::memcpy( mData.data() + offset, bytes.data(), bytes.size() );
		}

		template < IsStandard T >
		void Push( const T& data )
		{
			Set( data, mData.size() );
		}

		template < IsStandard T >
		T Pop()
		{
			constexpr std::size_t DataSize = sizeof( T );
			auto data = std::move( Read< T >( mData.size() - DataSize ) );
			Resize( mData.size() - DataSize );
			return data;
		}

		Byte* Data( size_t offset = 0 )
		{
			ASSERT( offset < mData.size() );
			return mData.data() + offset;
		}
		const Byte* Data( size_t offset = 0 ) const
		{
			ASSERT( offset < mData.size() );
			return mData.data() + offset;
		}

		size_t Size() const
		{
			return mData.size();
		}

		void Reserve( size_t new_size );
		void Resize( size_t new_size );

		Buffer CopyBytes( size_t size, size_t offset = 0 );

	public:
		operator bool() const
		{
			return mData.data() != nullptr and mData.size() > 0;
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
		std::vector< Byte > mData;
	};
} // namespace slc
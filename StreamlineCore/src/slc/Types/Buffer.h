#pragma once

#include "slc/Common/Base.h"

namespace slc {
	class Buffer
	{
	public:
		Buffer() = default;
		Buffer( std::nullptr_t )
		{}
		Buffer( Byte* data, size_t size )
			: mData( data ), mSize( size )
		{}
		Buffer( size_t size )
		{
			Allocate( size );
		}

		Buffer( const Buffer& buffer );
		Buffer( Buffer&& buffer ) noexcept;

		virtual ~Buffer()
		{
			Release();
		}

		Buffer& operator=( const Buffer& buffer );
		Buffer& operator=( Buffer&& buffer ) noexcept;

		static Buffer Copy( const void* data, size_t size );

	public:
		template < typename T, typename Self >
		decltype( auto ) As( this Self&& self )
		{
			using ReturnType = std::conditional_t< std::is_const_v< Self >, const T*, T* >;
			return reinterpret_cast< ReturnType >( std::forward< Self >( self ).mData );
		}

		template < IsStandard T >
		void Set( const T& data, size_t offset = 0 )
		{
			constexpr size_t DataSize = sizeof( T );
			if ( offset + DataSize > mSize )
				Resize( offset + DataSize );

			memcpy( mData + offset, &data, DataSize );
		}

		Byte* Data( size_t offset = 0 )
		{
			return mData + offset;
		}
		const Byte* Data( size_t offset = 0 ) const
		{
			return mData + offset;
		}

		size_t Size() const
		{
			return mSize;
		}
		void Resize( size_t newSize );

		Buffer CopyBytes( size_t size, size_t offset = 0 );


	public:
		operator bool() const
		{
			return mData != nullptr;
		}

		Byte& operator[]( size_t index )
		{
			return mData[ index ];
		}
		Byte operator[]( size_t index ) const
		{
			return mData[ index ];
		}

	protected:
		void Allocate( size_t size );
		void Release();

	protected:
		Byte* mData = nullptr;
		size_t mSize = 0;
	};
} // namespace slc
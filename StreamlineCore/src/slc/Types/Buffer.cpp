#include "Buffer.h"
#include <algorithm>
#include <cstring>

namespace slc {

	// ----- Constructors -----

	Buffer::Buffer( size_t size )
	{
		Resize( size );
	}

	Buffer::Buffer( const void* data, size_t size )
	{
		Resize( size );
		std::memcpy( mData.get(), data, size );
	}

	Buffer::Buffer( const Buffer& other )
	{
		Resize( other.mSize );
		std::memcpy( mData.get(), other.mData.get(), other.mSize );
	}

	Buffer::Buffer( Buffer&& other ) noexcept
		: mData( std::exchange( other.mData, nullptr ) )
		, mSize( std::exchange( other.mSize, 0 ) )
		, mCapacity( std::exchange( other.mCapacity, 0 ) )
	{
	}

	// ----- Assignment -----

	Buffer& Buffer::operator=( const Buffer& other )
	{
		if ( this != &other )
		{
			Resize( other.mSize );
			std::memcpy( mData.get(), other.mData.get(), other.mSize );
		}
		return *this;
	}

	Buffer& Buffer::operator=( Buffer&& other ) noexcept
	{
		mData = std::exchange( other.mData, nullptr );
		mSize = std::exchange( other.mSize, 0 );
		mCapacity = std::exchange( other.mCapacity, 0 );
		return *this;
	}

	// ----- Static Copy -----

	Buffer Buffer::Copy( const void* data, size_t size )
	{
		return Buffer( data, size );
	}

	// ----- Reserve / Resize -----

	void Buffer::Reserve( size_t new_capacity )
	{
		if ( new_capacity <= mCapacity )
			return;

		auto new_data = std::make_unique< Byte[] >( new_capacity );
		if ( mData )
			std::memcpy( new_data.get(), mData.get(), mSize );

		mData = std::move( new_data );
		mCapacity = new_capacity;
	}

	void Buffer::Resize( size_t new_size )
	{
		EnsureCapacity( new_size );
		if ( new_size > mSize )
			std::memset( mData.get() + mSize, 0, new_size - mSize );
		mSize = new_size;
	}

	// ----- Copy bytes -----

	Buffer Buffer::CopyBytes( size_t size, size_t offset ) const
	{
		if ( offset + size > mSize )
			throw std::runtime_error( "Buffer overflow" );
		return Buffer( mData.get() + offset, size );
	}

	// ----- View -----

	BufferView Buffer::View( size_t offset, size_t size )
	{
		return BufferView( *this, offset, size );
	}

} // namespace slc

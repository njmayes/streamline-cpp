#include "Buffer.h"

namespace slc {

	Buffer::Buffer( void* data, size_t size, bool owned )
		: mOwned{ owned }
	{
		if ( mOwned )
		{
			*this = Copy( data, size );
		}
		else
		{
			mData = static_cast< Byte* >( data );
			mSize = size;
			mCapacity = size;
		}
	}

	Buffer::Buffer( size_t size )
		: mOwned{ true }
	{
		Resize( size );
	}

	Buffer::Buffer( Buffer const& other )
		: Buffer( other.mData, other.mSize, other.mOwned )
	{
	}

	Buffer::Buffer( Buffer&& other ) noexcept
		: mData{ other.mData }
		, mSize{ other.mSize }
		, mCapacity{ other.mCapacity }
		, mOwned{ other.mOwned }
	{
	}

	Buffer& Buffer::operator=( Buffer const& other )
	{
		if ( other.mOwned )
		{
			*this = Copy( other.mData, other.mSize );
		}
		else
		{
			mData = static_cast< Byte* >( other.mData );
			mSize = other.mSize;
			mCapacity = other.mCapacity;
		}

		return *this;
	}

	Buffer& Buffer::operator=( Buffer&& other ) noexcept
	{
		mData = std::exchange( other.mData, nullptr );
		mSize = other.mSize;
		mCapacity = other.mCapacity;
		mOwned = other.mOwned;

		return *this;
	}

	Buffer::~Buffer()
	{
		if ( mOwned )
		{
			::operator delete[]( mData );
		}
	}

	Buffer Buffer::Copy( const void* data, size_t size )
	{
		auto buffer = Buffer{};
		buffer.Reserve( size );

		std::memcpy( buffer.mData, data, size );

		return buffer;
	}

	Buffer Buffer::CopyBytes( size_t size, size_t offset )
	{
		ASSERT( offset + size <= mSize, "Buffer overflow!" );
		return Buffer::Copy( Data( offset ), size );
	}

	void Buffer::Reserve( size_t new_size )
	{
		if ( !mOwned )
			return;

		if ( new_size < mCapacity )
			return;

		auto new_data = ::operator new[]( new_size );
		std::memset( new_data, 0, new_size );

		if ( mData )
		{
			auto copy_size = std::min( mSize, new_size );
			std::memcpy( new_data, mData, copy_size );
			::operator delete[]( mData );
		}

		mData = static_cast< Byte* >( new_data );
		mCapacity = new_size;
	}

	void Buffer::Resize( size_t new_size )
	{
		if ( !mOwned )
			return;

		if ( new_size > mSize )
		{
			auto new_data = ::operator new[]( new_size );
			std::memset( new_data, 0, new_size );

			if ( mData )
			{
				auto copy_size = std::min( mSize, new_size );
				std::memcpy( new_data, mData, copy_size );
				::operator delete[]( mData );
			}

			mData = static_cast< Byte* >( new_data );
			mCapacity = new_size;
		}

		mSize = new_size;
	}
} // namespace slc
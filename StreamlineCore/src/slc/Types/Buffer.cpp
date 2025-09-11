#include "Buffer.h"

namespace slc {

	Buffer Buffer::Copy( const void* data, size_t size )
	{
		auto span = std::span{ static_cast< const Byte* >( data ), size };
		auto buffer = Buffer{};
		buffer.mData.append_range( span );
		return buffer;
	}

	Buffer Buffer::CopyBytes( size_t size, size_t offset )
	{
		ASSERT( offset + size <= mData.size(), "Buffer overflow!" );
		return Buffer::Copy( Data( offset ), size );
	}

	void Buffer::Reserve( size_t new_size )
	{
		mData.reserve( new_size );
	}

	void Buffer::Resize( size_t new_size )
	{
		mData.resize( new_size );
	}
} // namespace slc
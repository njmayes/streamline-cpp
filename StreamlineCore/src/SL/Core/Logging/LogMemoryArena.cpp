#include "LogMemoryArena.h"

#include "SL/Core/Common/Profiling.h"

namespace sl {

	LogMemoryArena::LogMemoryArena( std::size_t size )
		: mBuffer{ MakeBox< char[] >( size ) }
		, mCapacity{ size }
	{
	}

	std::optional< MessageBuffer > LogMemoryArena::RequestBuffer( std::size_t size )
	{
		SL_PROFILE_FUNCTION();

		auto available = mCapacity - mUsed;
		if ( available < size )
			return std::nullopt;

		auto buffer = MessageBuffer( mBuffer.get() + mUsed, size );
		mUsed += size;
		return buffer;
	}

	void LogMemoryArena::ReleaseBuffers()
	{
		SL_PROFILE_FUNCTION();

		mUsed = 0;
	}
} // namespace sl

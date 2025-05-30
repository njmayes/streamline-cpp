#include "LogMemoryArena.h"

#include "slc/Common/Profiling.h"

namespace slc {

	LogMemoryArena::LogMemoryArena(std::size_t size)
		: mBuffer{ MakeUnique< char[] >(size) }
		, mCapacity{ size }
	{

	}

	std::optional<MessageBuffer> LogMemoryArena::RequestBuffer(std::size_t size)
	{
		SLC_PROFILE_FUNCTION();

		auto available = mCapacity - mUsed;
		if (available < size)
			return std::nullopt;

		auto buffer = MessageBuffer(mBuffer.get() + mUsed, size);
		mUsed += size;
		return buffer;
	}

	void LogMemoryArena::ReleaseBuffers()
	{
		SLC_PROFILE_FUNCTION();

		mUsed = 0;
	}
}

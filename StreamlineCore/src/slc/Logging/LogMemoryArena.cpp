#include "LogMemoryArena.h"

namespace slc {

	LogMemoryArena::LogMemoryArena()
		: mBuffer{ MakeImpl< char[] >(DefaultArenaSize) }
		, mSize{ DefaultArenaSize }
	{

	}

	std::optional<MessageBuffer> LogMemoryArena::RequestBuffer(std::size_t size)
	{
		auto new_tail = mTail + size;
		if (new_tail <= mHead)
			return std::nullopt;

		if (new_tail >= DefaultArenaSize)
		{
			new_tail %= DefaultArenaSize;
			if (new_tail >= mHead)
				return std::nullopt;
		}

		auto buffer = MessageBuffer(mBuffer.get() + mTail, size);
		mTail = new_tail;

		return buffer;
	}

	void LogMemoryArena::ReleaseBuffer(MessageBuffer buffer)
	{
		ASSERT(buffer.data() == (mBuffer.get() + mHead), "Arena is a queue, must release the oldest buffer first");

		auto new_head = mHead + buffer.size();
		ASSERT(new_head <= mTail, "Error, released buffer is larger than currently allocated space");

		mHead = new_head;
	}

	void LogMemoryArena::Reallocate()
	{
		ASSERT(mHead == mTail == 0, "Should never reallocate while there is still used memory");

		mSize = static_cast<std::size_t>(mSize * ReallocScaleFactor);
		mBuffer = MakeImpl< char[] >(mSize);
		mHead = 0;
		mTail = 0;
	}
}

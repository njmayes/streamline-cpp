#pragma once

#include "slc/Common/Base.h"

#include "Common.h"

namespace slc {

	class LogMemoryArena
	{
	public:
		SCONSTEXPR std::size_t DefaultArenaSize = 0x1000000;
		SCONSTEXPR double ReallocScaleFactor = 1.5;

	public:
		LogMemoryArena()
			: mBuffer{ MakeImpl< char[] >(DefaultArenaSize) }
		{

		}

		MessageBuffer RequestBuffer(std::size_t size)
		{
			auto new_tail = mTail + size;
			ASSERT(new_tail > mHead, "Arena ran out of space");

			if (new_tail >= DefaultArenaSize)
			{
				new_tail %= DefaultArenaSize;
				ASSERT(new_tail < mHead, "Arena ran out of space");
			}

			auto buffer = MessageBuffer(mBuffer.get() + mTail, size);
			mTail = new_tail;

			return buffer;
		}

		void ReleaseBuffer(MessageBuffer buffer)
		{
			ASSERT(buffer.data() == (mBuffer.get() + mHead), "Arena is a queue, must release the oldest buffer first");

			auto new_head = mHead + buffer.size();
			ASSERT(new_head <= mTail, "Error, released buffer is larger than currently allocated space");

			mHead = new_head;
		}

	private:
		Impl<char[]> mBuffer;
		std::size_t mHead = 0;
		std::size_t mTail = 0;
	};
}
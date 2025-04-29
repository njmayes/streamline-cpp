#include "LogMemoryArena.h"

namespace slc {

	LogMemoryArena::LogMemoryArena(std::size_t size)
		: mBuffer{ MakeImpl< char[] >(size) }
		, mCapacity{ size }
	{

	}

	std::optional<MessageBuffer> LogMemoryArena::RequestBuffer(std::size_t size)
	{
		if (Available() < size)
			return std::nullopt;

		std::size_t end = mHead + size;
		if (end <= mCapacity)
		{
			auto buffer = MessageBuffer(mBuffer.get() + mHead, size);
			mHead = (mHead + size) % mCapacity;
			mFull = mHead == mTail;
			return buffer;
		}
		else if (mTail > size)
		{
			auto buffer = MessageBuffer(mBuffer.get(), size);
			mHead = size;
			mFull = mHead == mTail;
			return buffer;
		}

		return std::nullopt;
	}

	void LogMemoryArena::ReleaseBuffer(MessageBuffer buffer)
	{
		ASSERT(mBuffer.get() + mTail == buffer.data(), "Arena is a stack, must release the oldest buffer first");
		ASSERT(mHead != mTail, "There is no currently in use memory");

		mTail = (mTail + buffer.size()) % mCapacity;
		mFull = false;
	}

	std::size_t LogMemoryArena::Available()
	{
		if (mFull)
			return 0;

		if (mHead >= mTail)
			return mCapacity - (mHead - mTail);

		return mTail - mHead;
	}
}

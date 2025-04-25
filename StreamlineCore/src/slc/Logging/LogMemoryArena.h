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
		LogMemoryArena();

		std::optional<MessageBuffer> RequestBuffer(std::size_t size);
		void ReleaseBuffer(MessageBuffer buffer);

		void Reallocate();

	private:
		Impl<char[]> mBuffer;
		std::size_t mSize;
		std::size_t mHead = 0;
		std::size_t mTail = 0;
	};
}
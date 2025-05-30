#pragma once

#include "slc/Common/Base.h"

#include "Common.h"

namespace slc {

	class LogMemoryArena
	{
	public:
		LogMemoryArena(std::size_t size);

		std::optional<MessageBuffer> RequestBuffer(std::size_t size);
		void ReleaseBuffers();

	private:
		Unique<char[]> mBuffer;
		std::size_t mCapacity;
		std::size_t mUsed = 0;
	};
}
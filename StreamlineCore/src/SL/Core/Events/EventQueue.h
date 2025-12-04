#pragma once

#include "EventModelAllocator.h"

#include "SL/Core/Threading/Flag.h"

namespace slc {

	struct EventQueue : public thread::Flag
	{
		std::deque< Event > events;
		ModelAllocator allocator;
	};
} // namespace slc
#pragma once

#include "EventModelAllocator.h"

namespace slc {

	struct EventQueue
	{
		std::deque< Event > events;
		ModelAllocator allocator;
	};
} // namespace slc
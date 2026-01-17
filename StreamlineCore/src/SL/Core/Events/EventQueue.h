#pragma once

#include "EventModelAllocator.h"

#include "SL/Core/Threading/Flag.h"

namespace sl {

	struct EventQueue
	{
		thread::Flag flag{};
		std::vector< Event > events;
		ModelAllocator allocator;
	};
} // namespace sl
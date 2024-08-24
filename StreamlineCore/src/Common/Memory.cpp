#include "Memory.h"

#include "Macros.h"

namespace slc {

	bool RefTracker::IsTracked(void* data)
	{
		ASSERT(data, "Data is null!");
		return sRefSet.count(data) != 0;
	}

	void RefTracker::AddToReferenceTracker(void* data)
	{
		ASSERT(data);
		sRefSet.insert(data);
	}

	void RefTracker::RemoveFromReferenceTracker(void* data)
	{
		ASSERT(data, "Data is null!");
		ASSERT(sRefSet.contains(data), "Ref not being tracked!");
		sRefSet.erase(data);
	}
}
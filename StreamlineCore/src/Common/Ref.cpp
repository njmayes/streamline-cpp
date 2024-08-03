#include "Ref.h"

namespace slc {

	bool RefTracker::IsTracked(void* data)
	{
		ASSERT(data, "Data is null!");
		return sRefSet.count(data) != 0;
	}

	void RefTracker::AddToTrackedRefs(void* data)
	{
		ASSERT(data);
		sRefSet.insert(data);
	}

	void RefTracker::RemoveFromTrackedRefs(void* data)
	{
		ASSERT(data, "Data is null!");
		ASSERT(sRefSet.count(data) != 0, "Ref not being tracked!");
		sRefSet.erase(data);
	}
}
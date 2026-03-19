#include "Memory.h"

#include "Macros.h"

namespace sl::detail {

	bool RefTracker::IsTracked( void const* data )
	{
		SL_ASSERT( data, "Data is null!" );
		return sRefSet.count( data ) != 0;
	}

	void RefTracker::AddToReferenceTracker( void const* data )
	{
		SL_ASSERT( data );
		sRefSet.insert( data );
	}

	void RefTracker::RemoveFromReferenceTracker( void const* data )
	{
		SL_ASSERT( data, "Data is null!" );
		SL_ASSERT( sRefSet.contains( data ), "Ref not being tracked!" );
		sRefSet.erase( data );
	}
} // namespace sl::detail
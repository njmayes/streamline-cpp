#include "Memory.h"

#include "Macros.h"

namespace sl::detail {

	bool RefTracker::IsTracked( void const* data )
	{
		SL_ASSERT( data, "Data should never be null here" );
		return sRefSet.count( data ) != 0;
	}

	void RefTracker::AddToReferenceTracker( void const* data )
	{
		SL_ASSERT( data );
		sRefSet.insert( data );
	}

	void RefTracker::RemoveFromReferenceTracker( void const* data )
	{
		SL_ASSERT( data, "Data should never be null here" );

		auto it = sRefSet.find( data );
		SL_ASSERT( it != sRefSet.end(), "Ref must always have been tracked" );
		sRefSet.erase( it );
	}
} // namespace sl::detail
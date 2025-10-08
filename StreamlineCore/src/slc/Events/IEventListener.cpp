#include "IEventListener.h"

#include "EventRuntime.h"

namespace slc {

	IEventListener::~IEventListener()
	{
		if ( mRuntime )
			mRuntime->DeregisterListener( this );
	}
} // namespace slc
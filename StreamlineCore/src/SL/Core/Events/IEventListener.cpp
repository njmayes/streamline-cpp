#include "IEventListener.h"

#include "EventRuntime.h"

namespace sl {

	IEventListener::~IEventListener()
	{
		if ( mRuntime )
			mRuntime->DeregisterListener( this );
	}
} // namespace sl
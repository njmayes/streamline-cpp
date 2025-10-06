#pragma

#include "Event.h"

#include "slc/Networking/Common.h"

namespace slc {

	struct NetworkInEvent : public EventBase
	{
		slc::net::Payload data;

		NetworkInEvent( slc::net::Payload payload )
			: data( std::move( payload ) )
		{}

		SLC_EVENT_DATA_TYPE( NetworkIn )
	};

	struct NetworkOutEvent : public EventBase
	{
		slc::net::Payload data;

		NetworkOutEvent( slc::net::Payload payload )
			: data( std::move( payload ) )
		{}

		SLC_EVENT_DATA_TYPE( NetworkOut )
	};

}
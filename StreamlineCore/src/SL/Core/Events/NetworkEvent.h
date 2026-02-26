#pragma

#include "Event.h"

#include "SL/Core/Networking/Common.h"

namespace sl {

	struct NetworkInEvent : public EventBase
	{
		sl::net::Payload data;

		NetworkInEvent( sl::net::Payload payload )
			: data( std::move( payload ) )
		{}

		SL_EVENT_DATA_TYPE( NetworkIn )
	};

	struct NetworkOutEvent : public EventBase
	{
		sl::net::Payload data;

		NetworkOutEvent( sl::net::Payload payload )
			: data( std::move( payload ) )
		{}

		SL_EVENT_DATA_TYPE( NetworkOut )
	};

}
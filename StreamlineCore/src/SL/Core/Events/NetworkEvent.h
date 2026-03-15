#pragma

#include "Event.h"

#include "SL/Core/Types/Buffer.h"

namespace sl {

	struct NetworkInEvent : public EventBase
	{
		sl::Buffer data;

		NetworkInEvent( sl::Buffer payload )
			: data( std::move( payload ) )
		{}

		SL_EVENT_DATA_TYPE( NetworkIn )
	};

	struct NetworkOutEvent : public EventBase
	{
		sl::Buffer data;

		NetworkOutEvent( sl::Buffer payload )
			: data( std::move( payload ) )
		{}

		SL_EVENT_DATA_TYPE( NetworkOut )
	};

}
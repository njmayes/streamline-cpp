#pragma

#include "Event.h"

#include "SL/Core/Types/Buffer.h"

namespace sl {

	struct NetworkInEvent
	{
		sl::Buffer data;

		NetworkInEvent( sl::Buffer payload )
			: data( std::move( payload ) )
		{}
	};

	struct NetworkOutEvent
	{
		sl::Buffer data;

		NetworkOutEvent( sl::Buffer payload )
			: data( std::move( payload ) )
		{}
	};

}
#pragma once

#include "slc/IO/KeyCodes.h"

#include "Event.h"

namespace slc {

	struct KeyPressedEvent : public EventBase
	{
		KeyCode key_code;
		bool repeat;

		KeyPressedEvent( KeyCode key, bool repeated )
			: key_code( key ), repeat( repeated )
		{}

		EVENT_DATA_TYPE( KeyPressed )
	};

	struct KeyReleasedEvent : public EventBase
	{
		KeyCode key_code;

		KeyReleasedEvent( KeyCode key )
			: key_code( key )
		{}

		EVENT_DATA_TYPE( KeyReleased )
	};

	struct KeyTypedEvent : public EventBase
	{
		KeyCode key_code;

		KeyTypedEvent( KeyCode key )
			: key_code( key )
		{}

		EVENT_DATA_TYPE( KeyTyped )
	};
} // namespace slc
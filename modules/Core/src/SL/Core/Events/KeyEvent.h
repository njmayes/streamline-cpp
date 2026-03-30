#pragma once

#include "SL/Core/IO/KeyCodes.h"

#include "Event.h"

namespace sl {

	struct KeyPressedEvent
	{
		KeyCode key_code;
		bool repeat;

		KeyPressedEvent( KeyCode key, bool repeated )
			: key_code( key ), repeat( repeated )
		{}
	};

	struct KeyReleasedEvent
	{
		KeyCode key_code;

		KeyReleasedEvent( KeyCode key )
			: key_code( key )
		{}
	};

	struct KeyTypedEvent
	{
		KeyCode key_code;

		KeyTypedEvent( KeyCode key )
			: key_code( key )
		{}
	};
} // namespace sl
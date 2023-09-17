#pragma once

#include "IO/KeyCodes.h"

#include "EventTypes.h"

namespace slc {

	struct KeyPressedEvent
	{
		KeyCode keyCode;
		bool repeat;

		KeyPressedEvent(KeyCode key, bool repeated) : keyCode(key), repeat(repeated) {}

		EVENT_DATA_TYPE(KeyPressed)
	};

	struct KeyReleasedEvent
	{
		KeyCode keyCode;

		KeyReleasedEvent(KeyCode key) : keyCode(key) {}

		EVENT_DATA_TYPE(KeyReleased)
	};

	struct KeyTypedEvent
	{
		KeyCode keyCode;

		KeyTypedEvent(KeyCode key) : keyCode(key) {}

		EVENT_DATA_TYPE(KeyTyped)
	};
}
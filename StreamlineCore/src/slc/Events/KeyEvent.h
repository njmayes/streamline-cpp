#pragma once

#include "slc/IO/KeyCodes.h"

#include "Event.h"

namespace slc {

	struct KeyPressedEvent : public EventBase
	{
		KeyCode keyCode;
		bool repeat;

		KeyPressedEvent(KeyCode key, bool repeated) : keyCode(key), repeat(repeated) {}

		EVENT_DATA_TYPE(KeyPressed)
	};

	struct KeyReleasedEvent : public EventBase
	{
		KeyCode keyCode;

		KeyReleasedEvent(KeyCode key) : keyCode(key) {}

		EVENT_DATA_TYPE(KeyReleased)
	};

	struct KeyTypedEvent : public EventBase
	{
		KeyCode keyCode;

		KeyTypedEvent(KeyCode key) : keyCode(key) {}

		EVENT_DATA_TYPE(KeyTyped)
	};
}
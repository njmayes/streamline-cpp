#pragma once

#include "slc/IO/MouseCodes.h"

#include "Event.h"

namespace slc {

	struct MouseMovedEvent : public EventBase
	{
		float mouseX, mouseY;

		MouseMovedEvent(float x, float y) : mouseX(x), mouseY(y) {}

		EVENT_DATA_TYPE(MouseMoved)
	};

	struct MouseScrolledEvent : public EventBase
	{
		float xOffset, yOffset;

		MouseScrolledEvent(float x, float y) : xOffset(x), yOffset(y) {}

		EVENT_DATA_TYPE(MouseScrolled)
	};

	struct MouseButtonPressedEvent : public EventBase
	{
		MouseCode button;

		MouseButtonPressedEvent(MouseCode mouse) : button(mouse) {}

		EVENT_DATA_TYPE(MouseButtonPressed)
	};

	struct MouseButtonReleasedEvent : public EventBase
	{
		MouseCode button;

		MouseButtonReleasedEvent(MouseCode mouse) : button(mouse) {}

		EVENT_DATA_TYPE(MouseButtonReleased)
	};
}
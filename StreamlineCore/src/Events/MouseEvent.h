#pragma once

#include "IO/MouseCodes.h"

#include "EventTypes.h"

namespace slc {

	struct MouseMovedEvent
	{
		float mouseX, mouseY;

		MouseMovedEvent(float x, float y) : mouseX(x), mouseY(y) {}

		EVENT_DATA_TYPE(MouseMoved)
	};

	struct MouseScrolledEvent
	{
		float xOffset, yOffset;

		MouseScrolledEvent(float x, float y) : xOffset(x), yOffset(y) {}

		EVENT_DATA_TYPE(MouseScrolled)
	};

	struct MouseButtonPressedEvent
	{
		MouseCode button;

		MouseButtonPressedEvent(MouseCode mouse) : button(mouse) {}

		EVENT_DATA_TYPE(MouseButtonPressed)
	};

	struct MouseButtonReleasedEvent
	{
		MouseCode button;

		MouseButtonReleasedEvent(MouseCode mouse) : button(mouse) {}

		EVENT_DATA_TYPE(MouseButtonReleased)
	};
}
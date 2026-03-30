#pragma once

#include "SL/Core/IO/MouseCodes.h"

#include "Event.h"

namespace sl {

	struct MouseMovedEvent
	{
		float mouse_x, mouse_y;

		MouseMovedEvent( float x, float y )
			: mouse_x( x ), mouse_y( y )
		{}
	};

	struct MouseScrolledEvent
	{
		float x_offset, y_offset;

		MouseScrolledEvent( float x, float y )
			: x_offset( x ), y_offset( y )
		{}
	};

	struct MouseButtonPressedEvent
	{
		MouseCode button;

		MouseButtonPressedEvent( MouseCode mouse )
			: button( mouse )
		{}
	};

	struct MouseButtonReleasedEvent
	{
		MouseCode button;

		MouseButtonReleasedEvent( MouseCode mouse )
			: button( mouse )
		{}
	};
} // namespace sl
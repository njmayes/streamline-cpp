#pragma once

#include "slc/IO/MouseCodes.h"

#include "Event.h"

namespace slc {

	struct MouseMovedEvent : public EventBase
	{
		float mouse_x, mouse_y;

		MouseMovedEvent( float x, float y )
			: mouse_x( x ), mouse_y( y )
		{}

		EVENT_DATA_TYPE( MouseMoved )
	};

	struct MouseScrolledEvent : public EventBase
	{
		float x_offset, y_offset;

		MouseScrolledEvent( float x, float y )
			: x_offset( x ), y_offset( y )
		{}

		EVENT_DATA_TYPE( MouseScrolled )
	};

	struct MouseButtonPressedEvent : public EventBase
	{
		MouseCode button;

		MouseButtonPressedEvent( MouseCode mouse )
			: button( mouse )
		{}

		EVENT_DATA_TYPE( MouseButtonPressed )
	};

	struct MouseButtonReleasedEvent : public EventBase
	{
		MouseCode button;

		MouseButtonReleasedEvent( MouseCode mouse )
			: button( mouse )
		{}

		EVENT_DATA_TYPE( MouseButtonReleased )
	};
} // namespace slc
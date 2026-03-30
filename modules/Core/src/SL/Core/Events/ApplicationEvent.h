#pragma once

#include "Event.h"

namespace sl {

	struct WindowResizeEvent
	{
		unsigned width, height;

		WindowResizeEvent( unsigned w, unsigned h )
			: width( w ), height( h )
		{}
	};

	struct WindowCloseEvent
	{
	};

	struct WindowFocusEvent
	{
	};

	struct WindowFocusLostEvent
	{
	};

	struct WindowMovedEvent
	{
		unsigned xpos, ypos;

		WindowMovedEvent( unsigned x, unsigned y )
			: xpos( x ), ypos( y )
		{}
	};


	struct AppTickEvent
	{
	};

	struct AppUpdateEvent
	{
	};

	struct AppRenderEvent
	{
	};
} // namespace sl
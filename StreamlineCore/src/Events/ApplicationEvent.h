#pragma once

#include "EventTypes.h"

namespace slc {

	struct WindowResizeEvent
	{
		unsigned width, height;

		WindowResizeEvent(unsigned w, unsigned h) : width(w), height(h) {}

		EVENT_DATA_TYPE(WindowResize)
	};

	struct WindowCloseEvent
	{
		EVENT_DATA_TYPE(WindowClose)
	};

	struct WindowFocusEvent
	{
		EVENT_DATA_TYPE(WindowFocus)
	};

	struct WindowFocusLostEvent
	{
		EVENT_DATA_TYPE(WindowLostFocus)
	};

	struct WindowMovedEvent
	{
		unsigned xpos, ypos;

		WindowMovedEvent(unsigned x, unsigned y) : xpos(x), ypos(y) {}

		EVENT_DATA_TYPE(WindowMoved)
	};


	struct AppTickEvent
	{
		EVENT_DATA_TYPE(AppTick)
	};

	struct AppUpdateEvent
	{
		EVENT_DATA_TYPE(AppUpdate)
	};

	struct AppRenderEvent
	{
		EVENT_DATA_TYPE(AppRender)
	};
}
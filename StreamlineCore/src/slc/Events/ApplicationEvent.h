#pragma once

#include "Event.h"

namespace slc {

	struct WindowResizeEvent : public EventBase
	{
		unsigned width, height;

		WindowResizeEvent(unsigned w, unsigned h) : width(w), height(h) {}

		EVENT_DATA_TYPE(WindowResize)
	};

	struct WindowCloseEvent : public EventBase
	{
		EVENT_DATA_TYPE(WindowClose)
	};

	struct WindowFocusEvent : public EventBase
	{
		EVENT_DATA_TYPE(WindowFocus)
	};

	struct WindowFocusLostEvent : public EventBase
	{
		EVENT_DATA_TYPE(WindowLostFocus)
	};

	struct WindowMovedEvent : public EventBase
	{
		unsigned xpos, ypos;

		WindowMovedEvent(unsigned x, unsigned y) : xpos(x), ypos(y) {}

		EVENT_DATA_TYPE(WindowMoved)
	};


	struct AppTickEvent : public EventBase
	{
		EVENT_DATA_TYPE(AppTick)
	};

	struct AppUpdateEvent : public EventBase
	{
		EVENT_DATA_TYPE(AppUpdate)
	};

	struct AppRenderEvent : public EventBase
	{
		EVENT_DATA_TYPE(AppRender)
	};
}
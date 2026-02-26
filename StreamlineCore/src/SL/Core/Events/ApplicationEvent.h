#pragma once

#include "Event.h"

namespace sl {

	struct WindowResizeEvent : public EventBase
	{
		unsigned width, height;

		WindowResizeEvent( unsigned w, unsigned h )
			: width( w ), height( h )
		{}

		SL_EVENT_DATA_TYPE( WindowResize )
	};

	struct WindowCloseEvent : public EventBase
	{
		SL_EVENT_DATA_TYPE( WindowClose )
	};

	struct WindowFocusEvent : public EventBase
	{
		SL_EVENT_DATA_TYPE( WindowFocus )
	};

	struct WindowFocusLostEvent : public EventBase
	{
		SL_EVENT_DATA_TYPE( WindowLostFocus )
	};

	struct WindowMovedEvent : public EventBase
	{
		unsigned xpos, ypos;

		WindowMovedEvent( unsigned x, unsigned y )
			: xpos( x ), ypos( y )
		{}

		SL_EVENT_DATA_TYPE( WindowMoved )
	};


	struct AppTickEvent : public EventBase
	{
		SL_EVENT_DATA_TYPE( AppTick )
	};

	struct AppUpdateEvent : public EventBase
	{
		SL_EVENT_DATA_TYPE( AppUpdate )
	};

	struct AppRenderEvent : public EventBase
	{
		SL_EVENT_DATA_TYPE( AppRender )
	};
} // namespace sl
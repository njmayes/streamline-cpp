#pragma once

#include "ApplicationEvent.h"
#include "KeyEvent.h"
#include "MouseEvent.h"
#include "NetworkEvent.h"

namespace sl {

	using WindowEvents = TypeList<
		WindowCloseEvent,
		WindowResizeEvent,
		WindowFocusEvent,
		WindowFocusLostEvent,
		WindowMovedEvent >;

	using AppEvents = TypeList<
		AppTickEvent,
		AppUpdateEvent,
		AppRenderEvent >;

	using KeyEvents = TypeList<
		KeyPressedEvent,
		KeyReleasedEvent,
		KeyTypedEvent >;

	using MouseEvents = TypeList<
		MouseButtonPressedEvent,
		MouseButtonReleasedEvent,
		MouseMovedEvent,
		MouseScrolledEvent >;

	using NetworkEvents = TypeList<
		NetworkInEvent,
		NetworkOutEvent >;


	using CoreEventList = TypeList<
		WindowEvents,
		AppEvents,
		KeyEvents,
		MouseEvents,
		NetworkEvents >;

	using Event = EventView< CoreEventList >;

	using MouseEvent = EventView< MouseEvents >;
	using KeyEvent = EventView< KeyEvents >;
	using AppEvent = EventView< AppEvents >;
	using WindowEvent = EventView< WindowEvents >;
	using NetworkEvent = EventView< NetworkEvents >;

} // namespace sl
#pragma once

#include "ApplicationEvent.h"
#include "KeyEvent.h"
#include "MouseEvent.h"
#include "NetworkEvent.h"

namespace sl {

	using CoreEventList = TypeList<
		WindowCloseEvent,
		WindowResizeEvent,
		WindowFocusEvent,
		WindowFocusLostEvent,
		WindowMovedEvent,
		AppTickEvent,
		AppUpdateEvent,
		AppRenderEvent,
		KeyPressedEvent,
		KeyReleasedEvent,
		KeyTypedEvent,
		MouseButtonPressedEvent,
		MouseButtonReleasedEvent,
		MouseMovedEvent,
		MouseScrolledEvent,
		NetworkInEvent,
		NetworkOutEvent >;

	using Event = EventView< CoreEventList >;
} // namespace sl
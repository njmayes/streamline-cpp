#pragma once

#include "Common/Functional.h"

#include "ApplicationEvent.h"
#include "KeyEvent.h"
#include "MouseEvent.h"

namespace slc {

	using AllEvents = TypeList<
		WindowCloseEvent, WindowResizeEvent, WindowFocusEvent, WindowFocusLostEvent, WindowMovedEvent,
		AppTickEvent, AppUpdateEvent, AppRenderEvent,
		KeyPressedEvent, KeyReleasedEvent, KeyTypedEvent,
		MouseButtonPressedEvent, MouseButtonReleasedEvent,
		MouseMovedEvent, MouseScrolledEvent
	>;

	template<typename T>
	concept IsEvent = AllEvents::Contains<T>;

#define SLC_BIND_EVENT_FUNC(fn) [this](IsEvent auto& event) -> bool { return this->fn(event); }

	struct Event
	{
		bool handled = false;
		EventTypeFlag type = EventType::None;
		AllEvents::VariantType data;

		template<IsEvent TEvent, typename... TArgs>
		void Init(TArgs&&... args) 
		{
			type = MakeBit(AllEvents::Index<TEvent>);
			data = TEvent(std::forward<TArgs>(args)...);
		}

		template<IsEvent T, typename Func> requires IsFunc<Func, bool, T&>
		void Dispatch(Func&& func)
		{
			if (type != T::GetStaticType())
				return;

			if (handled)
				return;

			handled = func(*std::get_if<T>(&data));
		}
	};

}
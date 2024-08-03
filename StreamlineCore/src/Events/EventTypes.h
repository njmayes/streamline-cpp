#pragma once

#include "Common/Base.h"

namespace slc {

	using EventTypeFlag = size_t;

	// Use enum in namespace over enum class so they can be used in bitwise operations but remain properly scoped.
	namespace EventType {

		static constexpr EventTypeFlag NextEventFlag(EventTypeFlag last) { return last << 1; }

		enum Flag : EventTypeFlag
		{	
			None = 0,

			WindowClose			= MakeBit(0), 
			WindowResize		= MakeBit(1), 
			WindowFocus			= MakeBit(2), 
			WindowLostFocus		= MakeBit(3), 
			WindowMoved			= MakeBit(4),

			AppTick				= MakeBit(5), 
			AppUpdate			= MakeBit(6), 
			AppRender			= MakeBit(7),

			KeyPressed			= MakeBit(8), 
			KeyReleased			= MakeBit(9), 
			KeyTyped			= MakeBit(10),

			MouseButtonPressed	= MakeBit(11), 
			MouseButtonReleased	= MakeBit(12), 
			MouseMoved			= MakeBit(13), 
			MouseScrolled		= MakeBit(14),

			/// <summary>
			/// <para>
			/// Use this as starting point when declaring a user-defined event flags enum for custom events.
			/// Event flags beyond this can be defined using the NextEventFlag function.
			/// </para>
			/// <para>
			/// E.g.
			/// </para>
			/// <para>
			/// enum MyEvents : EventTypeFlag
			/// </para>
			/// <para>
			/// {
			/// </para>
			/// <para>
			/// CustomEventA = NewEventStart,
			/// </para>
			/// <para>
			/// CustomEventB = NextEventFlag(CustomEventA)
			/// </para>
			/// <para>
			/// }
			/// </para>
			/// </summary>
			NewEventStart = MakeBit(15),

			//  Example:
			//	enum CustomEvents : EventTypeFlag
			//	{
			//		CustomEventA = NewEventStart,
			//		CustomEventB = NextEventFlag(CustomEventA),
			//	};
		};

		template<typename... T> requires (... and std::convertible_to<T, EventTypeFlag>)
		static constexpr EventTypeFlag BuildEventTypeMask(T&&... flags) 
		{ 
			if constexpr (sizeof...(T) == 0)
			{
				return EventType::None;
			}
			else
			{
				return (... | flags);
			}
		}
	}

	static constexpr EventTypeFlag EVENT_CATEGORY_APP = EventType::WindowClose | EventType::WindowResize | EventType::WindowFocus
		| EventType::WindowLostFocus | EventType::WindowMoved
		| EventType::AppTick | EventType::AppUpdate | EventType::AppRender;

	static constexpr EventTypeFlag EVENT_CATEGORY_KEY = EventType::KeyPressed | EventType::KeyReleased | EventType::KeyTyped;

	static constexpr EventTypeFlag EVENT_CATEGORY_MOUSE = EventType::MouseButtonPressed | EventType::MouseButtonReleased
		| EventType::MouseMoved | EventType::MouseScrolled;

	static constexpr EventTypeFlag EVENT_CATEGORY_INPUT = EVENT_CATEGORY_KEY | EVENT_CATEGORY_MOUSE;

#define EVENT_DATA_TYPE(type)	static EventTypeFlag GetStaticType() { return EventType::type; }
}
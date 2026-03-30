#pragma once

#include "SL/Core/Common/Base.h"

namespace sl {

	using EventTypeFlag = size_t;

#define SL_MAKE_EVENT_FLAG_BITS( i, event ) event = MakeBit( i )
#define SL_MAKE_EVENT_FLAGS( ... )                                                \
	namespace EventType {                                                          \
		enum : ::sl::EventTypeFlag                                                \
		{                                                                          \
			SL_FOR_EACH_I_SEP( SL_MAKE_EVENT_FLAG_BITS, SL_COMMA, __VA_ARGS__ ) \
		};                                                                         \
	}

	namespace EventType {
		enum : EventTypeFlag
		{
			None = 0,
		};
	}

	namespace detail {

		static constexpr EventTypeFlag NextEventFlag( EventTypeFlag last )
		{
			return last << 1;
		}

		template < typename... T >
			requires( ... and std::convertible_to< T, EventTypeFlag > )
		static constexpr EventTypeFlag BuildEventTypeMask( T&&... flags )
		{
			if constexpr ( sizeof...( T ) == 0 )
			{
				return EventType::None;
			}
			else
			{
				return ( ... | flags );
			}
		}
	} // namespace detail
	
	SL_MAKE_EVENT_FLAGS(
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled,
		NetworkIn, NetworkOut
	);

	namespace EventType {

		static constexpr EventTypeFlag EVENT_CATEGORY_APP = WindowClose | WindowResize | WindowFocus | WindowLostFocus | WindowMoved | AppTick | AppUpdate | AppRender;

		static constexpr EventTypeFlag EVENT_CATEGORY_KEY = KeyPressed | KeyReleased | KeyTyped;

		static constexpr EventTypeFlag EVENT_CATEGORY_MOUSE = MouseButtonPressed | MouseButtonReleased | MouseMoved | MouseScrolled;

		static constexpr EventTypeFlag EVENT_CATEGORY_NETWORK = NetworkIn | NetworkOut;

		static constexpr EventTypeFlag EVENT_CATEGORY_INPUT = EVENT_CATEGORY_KEY | EVENT_CATEGORY_MOUSE;

		static constexpr EventTypeFlag EVENT_CATEGORY_ALL = EVENT_CATEGORY_APP | EVENT_CATEGORY_KEY | EVENT_CATEGORY_MOUSE | EVENT_CATEGORY_NETWORK;
	} // namespace EventType

} // namespace sl
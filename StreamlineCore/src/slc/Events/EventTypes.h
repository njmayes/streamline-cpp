#pragma once

#include "slc/Common/Base.h"

namespace slc {

	using EventTypeFlag = size_t;


#define SLC_MAKE_EVENT_FLAG_BITS( i, event ) event = MakeBit( i )
#define SLC_MAKE_EVENT_FLAGS( ... )                                                \
	namespace EventType {                                                          \
		enum : ::slc::EventTypeFlag                                                \
		{                                                                          \
			SLC_FOR_EACH_I_SEP( SLC_MAKE_EVENT_FLAG_BITS, SLC_COMMA, __VA_ARGS__ ) \
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
	
	SLC_MAKE_EVENT_FLAGS(
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled,
		NetworkIn, NetworkOut,
		NewEventStart
	);

	namespace EventType {

		static constexpr EventTypeFlag EVENT_CATEGORY_APP = EventType::WindowClose | EventType::WindowResize | EventType::WindowFocus | EventType::WindowLostFocus | EventType::WindowMoved | EventType::AppTick | EventType::AppUpdate | EventType::AppRender;

		static constexpr EventTypeFlag EVENT_CATEGORY_KEY = EventType::KeyPressed | EventType::KeyReleased | EventType::KeyTyped;

		static constexpr EventTypeFlag EVENT_CATEGORY_MOUSE = EventType::MouseButtonPressed | EventType::MouseButtonReleased | EventType::MouseMoved | EventType::MouseScrolled;

		static constexpr EventTypeFlag EVENT_CATEGORY_INPUT = EVENT_CATEGORY_KEY | EVENT_CATEGORY_MOUSE;
	} // namespace EventType

#define SLC_EVENT_DATA_TYPE( type )             \
	static ::slc::EventTypeFlag GetStaticType() \
	{                                           \
		return EventType::type;                 \
	}
} // namespace slc
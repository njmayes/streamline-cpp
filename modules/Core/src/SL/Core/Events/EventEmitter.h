#pragma once

#include "EventDevice.h"

namespace sl {

	template <
		typename TEventList,
		EventRuntimeMode Mode,
		EventOrdering Ordering >
	class BasicEventRuntime;

	template < typename TRuntime >
	class BasicEventEmitter : public virtual BasicEventDevice< TRuntime >
	{
	public:
		using EventList = typename TRuntime::EventList;

		virtual void PollEvents()
		{

		}

		template < typename TEvent, typename... TArgs >
			requires IsRuntimeEvent< EventList, TEvent > &&
					 std::constructible_from< TEvent, TArgs... >
		void Emit( TArgs&&... args )
		{
			this->mRuntime->template Post< TEvent >( std::forward< TArgs >( args )... );
		}
	};

} // namespace sl
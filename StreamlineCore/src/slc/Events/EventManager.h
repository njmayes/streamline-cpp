#pragma once

#include "EventModelAllocator.h"

namespace slc {

	class IEventListener;

	/// <summary>
	/// The interface by which events are queued and handled. Use the Post(...) method to submit an event
	/// to be queued, which will then be handled at the start of the next frame in the Dispatch() method.
	///
	/// Listeners can be added by inheriting IEventListener which will automatically call Register/DeregisterListener
	/// in its constructor/desctructor respectively. This means they should generally be heap allocated objects,
	/// especially because addition and removal of listeners is queued to occur once per frame before dispatch.
	/// </summary>
	class EventManager
	{
	public:
		enum class ListenerType
		{
			Generic,
			App,
			ImGui
		};

	public:
		static void RegisterListener( IEventListener* listener, ListenerType type );
		static void DeregisterListener( IEventListener* listener, ListenerType type );

		template < IsEvent TEvent, typename... TArgs >
		static void Post( TArgs&&... args )
		{
			// Get event model instance from allocator. Event will be constructed in place inside model.
			EventModel< TEvent >& event_model = sState.model_allocator.NewModel< TEvent >( std::forward< TArgs >( args )... );

			// Add event to queue
			sState.event_queue.emplace_back( event_model );
		}

		static void Dispatch();

	private:
		struct EventManagerState
		{
			std::vector< Event > event_queue;
			EventModelAllocator model_allocator;

			IEventListener* app_listener = nullptr;
			IEventListener* imgui_listener = nullptr;
			std::vector< IEventListener* > generic_listeners;

			std::vector< IEventListener* > new_listeners;
			std::vector< IEventListener* > old_listeners;
		};

		inline static EventManagerState sState;
	};
} // namespace slc
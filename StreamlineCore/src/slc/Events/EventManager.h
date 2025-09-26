#pragma once

#include "EventModelAllocator.h"

#include <mutex>

namespace slc {

	class IEventListener;

	namespace detail {

		struct EventQueue
		{
			std::vector< Event > events;
			EventModelAllocator allocator;
		};

		struct EventManagerState
		{
			std::atomic_flag dispatching;

			std::mutex queue_lock;
			std::vector< EventQueue* > queue_registry{};

			IEventListener* app_listener = nullptr;
			IEventListener* imgui_listener = nullptr;
			std::vector< IEventListener* > generic_listeners;

			std::vector< IEventListener* > new_listeners;
			std::vector< IEventListener* > old_listeners;
		};
	} // namespace detail

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
			thread_local Box< detail::EventQueue > queue = MakeThreadEventQueue();

			// Wait in case we're currently dispatching events.
			sState.dispatching.wait( true );

			// Get event model instance from allocator. Event will be constructed in place inside model.
			EventModel< TEvent >& event_model = queue->allocator.NewModel< TEvent >( std::forward< TArgs >( args )... );

			// Add event to thread local queue
			queue->events.emplace_back( event_model );
		}

		static void Dispatch();
		static void DispatchAppListener( Event& e );
		static void DispatchImguiListener( Event& e );
		static void DispatchGenericListeners( Event& e );

	private:
		static Box< detail::EventQueue > MakeThreadEventQueue()
		{
			Box< detail::EventQueue > queue = MakeBox< detail::EventQueue >();

			std::unique_lock lock( sState.queue_lock );
			sState.queue_registry.push_back( queue.get() );

			return queue;
		}

		static std::vector< Event > MergeThreadQueues();
		static void CleanupThreadQueues();

	private:
		inline static detail::EventManagerState sState{};
	};
} // namespace slc
#pragma once

#include "EventQueue.h"

#include "ApplicationEvent.h"
#include "KeyEvent.h"
#include "MouseEvent.h"
#include "NetworkEvent.h"

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
	class EventRuntime
	{
	public:
		struct EventRuntimeState
		{
			std::atomic_flag dispatching;

			std::mutex queue_lock;
			std::vector< EventQueue* > queue_registry{};

			std::vector< IEventListener* > listeners;

			std::vector< IEventListener* > new_listeners;
			std::vector< IEventListener* > old_listeners;
		};

		struct EventRuntimeRegistry
		{
			std::mutex mutex;
			std::vector< EventRuntime* > runtimes;
		};

		EventRuntime()
		{
			std::scoped_lock lk( sRegistry.mutex );
			sRegistry.runtimes.push_back( this );
		}

		~EventRuntime()
		{
			std::scoped_lock lk( sRegistry.mutex );
			std::erase( sRegistry.runtimes, this );
		}

		EventRuntime( const EventRuntime& ) = delete;
		EventRuntime& operator=( const EventRuntime& ) = delete;

		EventRuntime( EventRuntime&& ) = default;
		EventRuntime& operator=( EventRuntime&& ) = default;

		static void RegisterListener( IEventListener* listener );
		static void DeregisterListener( IEventListener* listener );

		template < IsEvent TEvent, typename... TArgs >
		void Post( TArgs&&... args )
		{
			thread_local Box< EventQueue > queue = MakeThreadEventQueue();

			// Wait in case we're currently dispatching events.
			mState.dispatching.wait( true );

			// Get event model instance from allocator. Event will be constructed in place inside model.
			EventModel< TEvent >& event_model = queue->allocator.NewModel< TEvent >( std::forward< TArgs >( args )... );

			// Add event to thread local queue
			queue->events.emplace_back( event_model );
		}

		void Dispatch();

	private:
		Box< EventQueue > MakeThreadEventQueue();

		std::vector< Event > MergeThreadQueues();
		void CleanupThreadQueues();

	private:
		EventRuntimeState mState{};

		inline static EventRuntimeRegistry sRegistry;
	};
} // namespace slc
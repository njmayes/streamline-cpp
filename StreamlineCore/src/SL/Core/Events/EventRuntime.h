#pragma once

#include "EventQueue.h"
#include "IEventListener.h"

#include "ApplicationEvent.h"
#include "KeyEvent.h"
#include "MouseEvent.h"
#include "NetworkEvent.h"

namespace sl {

	/// <summary>
	/// The interface by which events are queued and handled. Use the Post(...) method to submit an event
	/// to be queued, which will then be handled on the next Dispatch() call.
	///
	/// Listeners can be used by inheriting a class from IEventListener and then by constructing it using
	/// CreateListener(...).
	/// 
	/// Runtimes are isolated, so any events posted to one runtime will not be seen by listeners on another.
	/// </summary>
	class EventRuntime
	{
	public:
		struct EventRuntimeState
		{
			thread::Flag dispatching{ thread::UnlockPolicy::NotifyAll };

			std::mutex queue_lock;
			std::vector< EventQueue* > queue_registry{};
			std::vector< Event > merged_queue;

			std::vector< IEventListener* > listeners;

			std::vector< IEventListener* > new_listeners;
			std::vector< IEventListener* > old_listeners;
		};

		template < IsEventListener T, typename... Args >
			requires std::constructible_from< T, Args... >
		Ref< T > CreateListener( Args&&... args )
		{
			auto listener = Ref< T >::Create( std::forward< Args >( args )... );
			listener->mRuntime = this;
			RegisterListener( listener.Data() );
			return listener;
		}

		template < IsEvent TEvent, typename... TArgs >
		void Post( TArgs&&... args )
		{
			thread_local Box< EventQueue > queue = MakeThreadEventQueue();

			auto lock = queue->flag.Lock();

			// Wait in case we're currently dispatching events.
			mState.dispatching.WaitUntil( false );

			// Get event model instance from allocator. Event will be constructed in place inside model.
			EventModel< TEvent >& event_model = queue->allocator.NewModel< TEvent >( std::forward< TArgs >( args )... );

			// Add event to thread local queue
			queue->events.emplace_back( event_model );
		}

		void Dispatch();

	private:
		Box< EventQueue > MakeThreadEventQueue();

		void MergeThreadQueues();
		void CleanupThreadQueues();

		void RegisterListener( IEventListener* listener );
		void DeregisterListener( IEventListener* listener );

		friend class IEventListener;
		friend class Application;

	private:
		EventRuntimeState mState{};
	};
} // namespace sl
#include "EventRuntime.h"

#include "slc/Common/Application.h"

#include "IEventListener.h"

namespace slc {

	void EventRuntime::Dispatch()
	{
		// Add any new listeners queued to start listening.
		mState.listeners.insert( mState.listeners.end(), mState.new_listeners.begin(), mState.new_listeners.end() );
		mState.new_listeners.clear();

		// Remove any listeners queued to remove.
		std::erase_if( mState.listeners, [ & ]( const IEventListener* listener ) { return std::ranges::contains( mState.old_listeners, listener ); } );
		mState.old_listeners.clear();

		auto dispatching = mState.dispatching.test_and_set();
		ASSERT( not dispatching, "Somehow we are dispatching from two places at once." );

		auto events = MergeThreadQueues();

		// Distribute events in the queue
		for ( Event& e : events )
		{
			for ( IEventListener* listener : mState.listeners | std::views::filter( [ & ]( IEventListener* listener ) { return listener->Accept( e ); } ) )
				listener->OnEvent( e );
		}

		// Clear down event queue and reset event model allocators.
		CleanupThreadQueues();

		mState.dispatching.clear();
		mState.dispatching.notify_all();
	}

	Box< EventQueue > EventRuntime::MakeThreadEventQueue()
	{
		Box< EventQueue > queue = MakeBox< EventQueue >();

		std::unique_lock lock( mState.queue_lock );
		mState.queue_registry.push_back( queue.get() );

		return queue;
	}

	std::vector< Event > EventRuntime::MergeThreadQueues()
	{
		auto all_events = std::vector< Event >{};

		std::unique_lock lock( mState.queue_lock );
		for ( auto queue : mState.queue_registry )
		{
			for ( auto e : queue->events )
				all_events.push_back( e );
		}

		return all_events;
	}

	void EventRuntime::CleanupThreadQueues()
	{
		std::unique_lock lock( mState.queue_lock );
		for ( auto queue : mState.queue_registry )
		{
			queue->events.clear();
		}
	}

	void EventRuntime::RegisterListener( IEventListener* listener )
	{
		for ( auto runtime : sRegistry.runtimes )
			runtime->mState.new_listeners.push_back( listener );
	}

	void EventRuntime::DeregisterListener( IEventListener* listener )
	{
		for ( auto runtime : sRegistry.runtimes )
			runtime->mState.old_listeners.push_back( listener );
	}
} // namespace slc
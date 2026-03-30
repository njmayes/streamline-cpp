#include "EventRuntime.h"

#include "SL/Core/Common/Application.h"

#include "IEventListener.h"

namespace sl {

	static constexpr std::size_t QUEUE_MIN_SIZE = 64;

	void EventRuntime::Dispatch()
	{
		// Add any new listeners queued to start listening.
		mState.listeners.insert( mState.listeners.end(), mState.new_listeners.begin(), mState.new_listeners.end() );
		mState.new_listeners.clear();

		// Remove any listeners queued to remove.
		std::erase_if( mState.listeners, [ & ]( const IEventListener* listener ) { return std::ranges::contains( mState.old_listeners, listener ); } );
		mState.old_listeners.clear();

		MergeThreadQueues();

		// Distribute events in the queue
		for ( Event& e : mState.merged_queue )
		{
			for ( IEventListener* listener : mState.listeners | std::views::filter( [ & ]( IEventListener* listener ) { return listener->Accept( e ); } ) )
				listener->OnEvent( e );
		}

		// Clear down event queue and reset event model allocators.
		CleanupThreadQueues();
	}

	Box< EventQueue > EventRuntime::MakeThreadEventQueue()
	{
		Box< EventQueue > queue = MakeBox< EventQueue >();

		// Hopefully avoid some allocations later.
		queue->events.reserve( QUEUE_MIN_SIZE );

		std::unique_lock lock( mState.queue_lock );
		mState.queue_registry.push_back( queue.get() );

		return queue;
	}

	void EventRuntime::MergeThreadQueues()
	{
		// Should this be before or after lock? Before could in theory be inaccurate but wont hold lock while reserving.
		// But if it's inaccurate, might end up reallocating during merge if new queue is added.
		// Unlikely to ever matter - but worth thinking about.
		mState.merged_queue.reserve( mState.queue_registry.size() * QUEUE_MIN_SIZE );

		std::unique_lock lock( mState.queue_lock );

		for ( auto queue : mState.queue_registry )
		{
			auto lock = queue->flag.Lock();

			// When gcc add append_range use that instead.
			mState.merged_queue.insert( mState.merged_queue.end(), queue->events.begin(), queue->events.end() );

			queue->events.clear();
		}
	}

	void EventRuntime::CleanupThreadQueues()
	{
		std::unique_lock lock( mState.queue_lock );
		for ( auto queue : mState.queue_registry )
		{
			auto lock = queue->flag.Lock();
			queue->allocator.Flush();
		}

		mState.merged_queue.clear();
	}

	void EventRuntime::RegisterListener( IEventListener* listener )
	{
		std::scoped_lock lk( mState.queue_lock );
		mState.new_listeners.push_back( listener );
	}

	void EventRuntime::DeregisterListener( IEventListener* listener )
	{
		std::scoped_lock lk( mState.queue_lock );
		mState.old_listeners.push_back( listener );
	}
} // namespace sl
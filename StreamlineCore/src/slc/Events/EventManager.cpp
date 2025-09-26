#include "EventManager.h"

#include "slc/Common/Application.h"

#include "IEventListener.h"

namespace slc {

	void EventManager::Dispatch()
	{
		// Add any new listeners queued to start listening.
		sState.generic_listeners.insert( sState.generic_listeners.end(), sState.new_listeners.begin(), sState.new_listeners.end() );
		sState.new_listeners.clear();

		// Remove any listeners queued to remove.
		std::erase_if( sState.generic_listeners, [ & ]( const IEventListener* listener ) { return std::ranges::contains( sState.old_listeners, listener ); } );
		sState.old_listeners.clear();

		auto dispatching = sState.dispatching.test_and_set();
		ASSERT( not dispatching, "Somehow we are dispatching from two places at once." );

		auto events = MergeThreadQueues();

		// Distribute events in the queue
		for ( Event& e : events )
		{
			// Handle app events first
			DispatchAppListener( e );

			// Handle imgui events next
			DispatchImguiListener( e );

			// Handle any generic listeners that accept this event type.
			DispatchGenericListeners( e );
		}

		// Clear down event queue and reset event model allocators.
		CleanupThreadQueues();

		sState.dispatching.clear();
		sState.dispatching.notify_all();
	}

	void EventManager::DispatchAppListener( Event& e )
	{
		if ( not sState.app_listener )
			return;

		if ( sState.app_listener->Accept( e ) )
			sState.app_listener->OnEvent( e );
	}

	void EventManager::DispatchImguiListener( Event& e )
	{
		if ( not sState.imgui_listener )
			return;

		if ( sState.imgui_listener->Accept( e ) )
			sState.imgui_listener->OnEvent( e );
	}

	void EventManager::DispatchGenericListeners( Event& e )
	{
		for ( IEventListener* listener : sState.generic_listeners | std::views::filter( [ & ]( IEventListener* listener ) { return listener->Accept( e ); } ) )
			listener->OnEvent( e );
	}

	std::vector< Event > EventManager::MergeThreadQueues()
	{
		auto all_events = std::vector< Event >{};

		std::unique_lock lock( sState.queue_lock );
		for ( auto queue : sState.queue_registry )
		{
			for ( auto e : queue->events )
				all_events.push_back( e );
		}

		return all_events;
	}

	void EventManager::CleanupThreadQueues()
	{
		std::unique_lock lock( sState.queue_lock );
		for (auto queue : sState.queue_registry)
		{
			queue->events.clear();
			queue->allocator.Flush();
		}
	}

	void EventManager::RegisterListener( IEventListener* listener, ListenerType type )
	{
		switch ( type )
		{
			case ListenerType::Generic:
			{
				// Some events may create a new listener while we're iterating through the listeners, so postpone addition till start of new frame.
				sState.new_listeners.emplace_back( listener );
				break;
			}
			case ListenerType::App:
			{
				sState.app_listener = listener;
				break;
			}
			case ListenerType::ImGui:
			{
				sState.imgui_listener = listener;
				break;
			}
		}
	}

	void EventManager::DeregisterListener( IEventListener* listener, ListenerType type )
	{
		switch ( type )
		{
			case ListenerType::Generic:
			{
				sState.old_listeners.emplace_back( listener );
				break;
			}
			case ListenerType::App:
			{
				sState.app_listener = nullptr;
				break;
			}
			case ListenerType::ImGui:
			{
				sState.imgui_listener = nullptr;
				break;
			}
		}
	}
} // namespace slc
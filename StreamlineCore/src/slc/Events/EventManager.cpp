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

		std::swap( sState.queue, sState.new_queue );

		// Distribute events in the queue
		for ( Event& e : sState.queue.events )
		{
			// Handle app events first
			DispatchAppListener( e );

			// Handle imgui events next
			DispatchImguiListener( e );

			// Handle any generic listeners that accept this event type.
			DispatchGenericListeners( e );
		}

		// Clear down event queue and reset event model allocators.
		sState.queue.events.clear();
		sState.queue.allocator.Flush();
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
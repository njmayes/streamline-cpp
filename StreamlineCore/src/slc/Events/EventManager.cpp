#include "EventManager.h"

#include "slc/Common/Application.h"

#include "IEventListener.h"

namespace slc {

	void EventManager::Dispatch()
	{
		// Add any new listeners queued to start listening.
		sState.genericListeners.insert(sState.genericListeners.end(), sState.newListeners.begin(), sState.newListeners.end());
		sState.newListeners.clear();

		// Remove any listeners queued to remove.
		std::erase_if(sState.genericListeners,
			[&](const IEventListener* listener) { return std::ranges::contains(sState.oldListeners, listener); });
		sState.oldListeners.clear();

		// Distribute events in the queue
		for (Event& e : sState.eventQueue)
		{
			// Handle app events first
			if (sState.appListener->Accept(e))
				sState.appListener->OnEvent(e);

			// Handle imgui events next
			if (sState.imGuiListener->Accept(e))
				sState.imGuiListener->OnEvent(e);

			// Handle any generic listeners that accept this event type.
			for (IEventListener* listener : sState.genericListeners.Where([&](IEventListener* listener) { return listener->Accept(e); }))
				listener->OnEvent(e);
		}

		// Clear down event queue and reset event model allocators.
		sState.eventQueue.clear();
		sState.modelAllocator.Flush();
	}

	void EventManager::RegisterListener(IEventListener* listener, ListenerType type)
	{
		switch (type)
		{
		case ListenerType::Generic:
		{
			// Some events may create a new listener while we're iterating through the listeners, so postpone addition till start of new frame.
			sState.newListeners.emplace_back(listener);
			break;
		}
		case ListenerType::App:
		{
			sState.appListener = listener;
			break;
		}
		case ListenerType::ImGui:
		{
			sState.imGuiListener = listener;
			break;
		}
		}
	}

	void EventManager::DeregisterListener(IEventListener* listener, ListenerType type)
	{
		switch (type)
		{
		case ListenerType::Generic:
		{
			sState.oldListeners.emplace_back(listener);
			break;
		}
		case ListenerType::App:
		{
			sState.appListener = nullptr;
			break;
		}
		case ListenerType::ImGui:
		{
			sState.imGuiListener = nullptr;
			break;
		}
		}
	}
}
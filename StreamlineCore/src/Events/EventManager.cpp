#include "EventManager.h"

#include "Common/Application.h"

#include "IEventListener.h"

namespace slc {

	void EventManager::Dispatch()
	{
		sGenericListeners.insert(sGenericListeners.end(), sNewListeners.begin(), sNewListeners.end());
		sNewListeners.clear();

		std::erase_if(sGenericListeners,
			[&](const IEventListener* listener) { return std::ranges::find(sOldListeners, listener) != sOldListeners.end(); });
		sOldListeners.clear();

		while (!sEventQueue.empty())
		{
			Event& e = sEventQueue.front();

			// Handle app events first
			if (sAppListener->Accept(e))
				sAppListener->OnEvent(e);

			// Handle imgui events next
			if (sImGuiListener->Accept(e))
				sImGuiListener->OnEvent(e);

			auto filteredListeners = std::views::common(sGenericListeners) |
				std::views::filter([&](IEventListener* listener) { return listener->Accept(e); });

			for (IEventListener* listener : filteredListeners)
				listener->OnEvent(e);

			sEventQueue.pop_front();
		}
	}

	void EventManager::RegisterListener(IEventListener* listener, ListenerType type)
	{
		switch (type)
		{
		case ListenerType::Generic:
		{
			// Some events may create a new listener while we're iterating through the listeners, so postpone addition till start of new app loop.
			sNewListeners.emplace_back(listener);
			break;
		}
		case ListenerType::App:
		{
			sAppListener = listener;
			break;
		}
		case ListenerType::ImGui:
		{
			sImGuiListener = listener;
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
			sOldListeners.emplace_back(listener);
			break;
		}
		case ListenerType::App:
		{
			sAppListener = nullptr;
			break;
		}
		case ListenerType::ImGui:
		{
			sImGuiListener = nullptr;
			break;
		}
		}
	}
}
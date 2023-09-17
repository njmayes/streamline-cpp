#include "pch.h"
#include "EventManager.h"

#include "Common/Application.h"

#include "IEventListener.h"

namespace slc {

	void EventManager::Dispatch()
	{
		while (!sEventQueue.empty())
		{
			Event& e = sEventQueue.front();

			for (IEventListener* listener : sGenericListeners)
			{
				if (!e.handled && listener->Accept(e.type))
					listener->OnEvent(e);
			}

			sEventQueue.pop();
		}
	}

	void EventManager::RegisterListener(IEventListener* listener, ListenerType type)
	{
		switch (type)
		{
		case ListenerType::Generic:
		{
			// Some events may create a new listener while we're iterating through the listeners, so postpone addition till start of new game loop.
			Application::SubmitActionToMainThread([&, listener]() { sGenericListeners.emplace_back(listener); });
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
			Application::SubmitActionToMainThread([&, listener]() { std::erase(sGenericListeners, listener); });
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
#pragma once

#include <queue>

#include "Event.h"

namespace slc {

	class IEventListener;

	enum class ListenerType
	{
		Generic,
		App,
		ImGui
	};

	class EventManager
	{
	public:
		static void RegisterListener(IEventListener* listener, ListenerType type);
		static void DeregisterListener(IEventListener* listener, ListenerType type);

		template<IsEvent TEvent, typename... TArgs>
		static void Post(TArgs&&... args)
		{
			Event& e = sEventQueue.emplace();
			e.Init<TEvent>(std::forward<TArgs>(args)...);
		}

		static void Dispatch();

	private:
		inline static std::queue<Event> sEventQueue;
		
		inline static IEventListener* sAppListener = nullptr;
		inline static IEventListener* sImGuiListener = nullptr;
		inline static std::vector<IEventListener*> sGenericListeners;

		inline static std::vector<IEventListener*> sNewListeners;
		inline static std::vector<IEventListener*> sOldListeners;
	};
}
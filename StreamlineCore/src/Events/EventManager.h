#pragma once

#include "Containers/Vector.h"
#include "Containers/Deque.h"

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
			Event& e = sEventQueue.emplace_back();
			e.Init<TEvent>(std::forward<TArgs>(args)...);
		}

		static void Dispatch();

	private:
		inline static Deque<Event> sEventQueue;
		
		inline static IEventListener* sAppListener = nullptr;
		inline static IEventListener* sImGuiListener = nullptr;
		inline static Vector<IEventListener*> sGenericListeners;

		inline static Vector<IEventListener*> sNewListeners;
		inline static Vector<IEventListener*> sOldListeners;
	};
}
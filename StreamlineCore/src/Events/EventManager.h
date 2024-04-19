#pragma once

#include "EventModelAllocator.h"

#include "Collections/Deque.h"
#include "Collections/Vector.h"

namespace slc {

	class IEventListener;

	/// <summary>
	/// The interface by which events are queued and handled. Use the Post(...) method to submit an event
	/// to be queued, which will then be handled at the start of the next frame in the Dispatch() method.
	/// 
	/// Listeners can be added by inheriting IEventListener which will automatically call Register/DeregisterListener
	/// in its constructor/desctructor respectively. This means they should generally be heap allocated objects,
	/// especially because addition and removal of listeners is queued to occur once per frame before dispatch.
	/// </summary>
	class EventManager
	{
	public:
		enum class ListenerType
		{
			Generic,
			App,
			ImGui
		};

	public:
		static void RegisterListener(IEventListener* listener, ListenerType type);
		static void DeregisterListener(IEventListener* listener, ListenerType type);

		template<IsEvent TEvent, typename... TArgs>
		static void Post(TArgs&&... args)
		{
			// Get event model instance from allocator. Event will be constructed in place inside model.
			EventModel<TEvent>& eventModel = sState.modelAllocator.NewModel<TEvent>(std::forward<TArgs>(args)...);

			// Add event to queue
			sState.eventQueue.emplace_back(eventModel);
		}

		static void Dispatch();

	private:
		struct EventManagerState
		{
			Vector<Event> eventQueue;
			EventModelAllocator modelAllocator;

			IEventListener* appListener = nullptr;
			IEventListener* imGuiListener = nullptr;
			Vector<IEventListener*> genericListeners;

			Vector<IEventListener*> newListeners;
			Vector<IEventListener*> oldListeners;
		};

		inline static EventManagerState sState;
	};
}
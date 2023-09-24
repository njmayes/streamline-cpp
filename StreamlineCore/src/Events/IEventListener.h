#pragma once

#include "EventManager.h"

namespace slc {

	class IEventListener
	{
	public:
		IEventListener() { EventManager::RegisterListener(this, mType); }
		virtual ~IEventListener() { EventManager::DeregisterListener(this, mType); }

		virtual constexpr EventTypeFlag GetListeningEvents() const = 0;
		virtual void OnEvent(Event& e) = 0;
		virtual void SetEventCondition(Predicate<>&& condition) { mAcceptCondition = std::move(condition); }

		bool Accept(Event& event) const
		{ 
			//   Not already handled		Valid Event Type				Satisfies additional condition
			return !event.handled && (GetListeningEvents() & event.type) && mAcceptCondition();
		}

	private:
		IEventListener(ListenerType type)
			: mType(type)
		{ 
			EventManager::RegisterListener(this, mType);
		}

		friend class Application;
		friend class ImGuiHandler;

	private:
		ListenerType mType = ListenerType::Generic;
		Predicate<> mAcceptCondition = [](){ return true; };
	};

#define LISTENING_EVENTS(...)	static constexpr EventTypeFlag GetStaticType() { return EXPAND_EVENTS(__VA_ARGS__); }\
								virtual constexpr EventTypeFlag GetListeningEvents() const override { return GetStaticType(); }
}
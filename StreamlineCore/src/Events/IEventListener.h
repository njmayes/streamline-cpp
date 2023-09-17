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
		virtual void SetEventCondition(Predicate<> condition) { mAcceptCondition = condition; }

		bool Accept(EventTypeFlag type) const
		{ 
			bool acceptType = (GetListeningEvents() & type);
			return acceptType && mAcceptCondition();
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
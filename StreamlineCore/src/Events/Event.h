#pragma once

#include "EventTypes.h"

namespace slc {

	// Forward declare for use in IsEvent concept
	struct EventBase;

	/// <summary>
	/// Event types must derive from EventBase and implement a static function
	/// that returns the event's type. Use the EVENT_DATA_TYPE(type) macro
	/// to help implement this.
	/// </summary>
	template<typename T>
	concept IsEvent = std::derived_from<T, EventBase> and requires 
	{
		{ T::GetStaticType() } -> std::same_as<EventTypeFlag>;
	};

	/// <summary>
	/// The base class for all events that contains metadata like the event type and 
	/// whether the event has been handled. Can only be accessed from friend classes.
	/// </summary>
	struct EventBase
	{
	private:
		bool handled = false;
		EventTypeFlag type = EventType::None;

		friend class Event;
		friend class EventManager;

		template<IsEvent T>
		friend struct EventModel;
	};

	/// <summary>
	/// Type erasure interface base class with access methods for metadata
	/// </summary>
	struct EventConcept
	{
		virtual ~EventConcept() {}
		virtual bool Handled() const = 0;
		virtual void SetHandled() = 0;
		virtual EventTypeFlag Type() const = 0;
	};


	/// <summary>
	/// Implementation of type erased event model for a given event type.
	/// Stores event data and implements interface to access event metadata.
	/// </summary>
	/// <typeparam name="T">The event type this class models</typeparam>
	template<IsEvent T>
	struct EventModel : public EventConcept
	{
		template<typename... Args>
		EventModel(Args&&... args)
			: object(std::forward<Args>(args)...) 
		{
			object.type = T::GetStaticType();
		}

		bool Handled() const override { return object.handled; }
		void SetHandled() override { object.handled = true; }
		EventTypeFlag Type() const override { return object.type; }

		T object;
	};

#define SLC_BIND_EVENT_FUNC(fn) [this](IsEvent auto& event) -> bool { return this->fn(event); }
	
	/// <summary>
	/// Type erased event class. Contains a pointer to the base event concept
	/// which is allocated externally in EventModelAllocator and passed in.
	/// Used to dispatch the event to a passed function.
	/// </summary>
	class Event
	{
	public:
		template<IsEvent T>
		Event(EventModel<T>& event) : mImpl(&event)
		{
		}

		~Event()
		{
		}

		/// <summary>
		/// Pass a predicate invocable object that takes an event type reference
		/// as a parameter. Use the SLC_BIND_EVENT_FUNC macro for assistance
		/// binding member functions.
		/// </summary>
		template<IsEvent T, IsPredicate<T&> Func>
		void Dispatch(Func&& func) noexcept(noexcept(func(std::declval<T&>()))) 
		{
			if (Handled())
				return;

			if (Type() != T::GetStaticType())
				return;

			EventModel<T>* pImpl = dynamic_cast<EventModel<T>*>(mImpl);
			pImpl->object.handled = func(pImpl->object);
		}
		
		EventTypeFlag Type() const { return mImpl->Type(); }
		bool Handled() const { return mImpl->Handled(); }
		void SetHandled() { mImpl->SetHandled(); }

	private:
		EventConcept* mImpl;
	};

}
#pragma once

#include "EventTypes.h"

namespace slc {

	/// <summary>
	/// The base class for all events that contains metadata like the event type and 
	/// whether the event has been handled. Metadata only accessible from EventConcept.
	/// </summary>
	struct EventBase
	{
	private:
		bool handled = false;
		EventTypeFlag type = EventType::None;

		friend struct EventConcept;
	};

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
	/// Type erasure interface base class with access methods for metadata.
	/// </summary>
	struct EventConcept
	{
		EventConcept(EventBase* base)
			: metadata(base) {}

		virtual ~EventConcept() = default;

		bool Handled() const { return metadata->handled; }
		EventTypeFlag Type() const { return metadata->type; }

	protected:
		template<IsEvent T>
		void SetType() { metadata->type = T::GetStaticType(); }

		void SetHandled(bool handled = true) { metadata->handled = handled; }

	private:
		EventBase* metadata;

		friend class Event;
	};


	/// <summary>
	/// Implementation of type erased event model for a given event type.
	/// Stores event data and implements interface to access event metadata.
	/// </summary>
	/// <typeparam name="T">The event type this class models</typeparam>
	template<IsEvent T>
	struct EventModel final : public EventConcept
	{
		template<typename... Args>
		EventModel(Args&&... args)
			: EventConcept(&object), object(std::forward<Args>(args)...) 
		{
			SetType<T>();
		}

	private:
		T object;

		friend class Event;
	};

#define SLC_BIND_EVENT_FUNC(fn) [this](::slc::IsEvent auto& event) -> bool { return this->fn(event); }
	
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

		~Event() = default;

		/// <summary>
		/// Pass a predicate invocable object that takes an event type reference
		/// as a parameter. Use the SLC_BIND_EVENT_FUNC macro for assistance
		/// binding member functions.
		/// </summary>
		template<IsEvent T, IsPredicate<T&> Func>
		void Dispatch(Func&& func) noexcept(noexcept(func(std::declval<T&>()))) 
		{
			if (IsHandled())
				return;

			if (GetType() != T::GetStaticType())
				return;

			EventModel<T>* pImpl = static_cast<EventModel<T>*>(mImpl);
			bool handled = func(pImpl->object);

			mImpl->SetHandled(handled);
		}
		
		bool IsHandled() const { return mImpl->Handled(); }
		EventTypeFlag GetType() const { return mImpl->Type(); }

	private:
		// Handled flag should not be set by the user manually. It should be handled using the Dispatch method instead.
		// Exception is that ImGuiController has specialised behaviour and needs to be able to set the handled flag
		// more generally than for individual event types.
		void SetHandled(bool handled = true) { mImpl->SetHandled(handled); }

		friend class ImGuiController;

	private:
		EventConcept* mImpl;
	};

}
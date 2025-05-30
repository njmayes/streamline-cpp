#pragma once

#include "Event.h"

#include "ApplicationEvent.h"
#include "KeyEvent.h"
#include "MouseEvent.h"

#include "slc/Allocators/LinearAllocator.h"

namespace slc {

	namespace EventList
	{
		using All = TypeList<
			WindowCloseEvent, WindowResizeEvent, WindowFocusEvent, WindowFocusLostEvent, WindowMovedEvent,
			AppTickEvent, AppUpdateEvent, AppRenderEvent,
			KeyPressedEvent, KeyReleasedEvent, KeyTypedEvent,
			MouseButtonPressedEvent, MouseButtonReleasedEvent,
			MouseMovedEvent, MouseScrolledEvent
		>;
	}

	/// <summary>
	/// Allocates and constructs event models for any given event type.
	/// </summary>
	class EventModelAllocator
	{
	private:
		using TypeName = std::string_view;
		SCONSTEXPR size_t DefaultModelChunkSize = 4;

		struct ModelAllocator
		{
			Unique<IAllocator> allocator = nullptr;
			size_t remaining = 0;

			template<IsEvent T>
			ModelAllocator(Unique<LinearAllocator<EventModel<T>>> alloc)
				: allocator(std::move(alloc)), remaining(allocator->MaxSize()) {}
		};

		using InternalAllocatorElement = std::pair<TypeName, ModelAllocator>;
		using InternalAllocatorArray = std::array<InternalAllocatorElement, EventList::All::Size>;
		using InternalAllocatorMap = std::map<TypeName, ModelAllocator>;

		template<size_t I> requires (I < EventList::All::Size)
		static InternalAllocatorElement BuildEventAllocator()
		{
			using Type = EventList::All::Type<I>;
			return std::make_pair(TypeTraits<Type>::Name, MakeUnique<LinearAllocator<EventModel<Type>>>(DefaultModelChunkSize));
		}

		template<size_t... Is> 
		static InternalAllocatorArray BuildAllEventAllocators(std::index_sequence<Is...>)
		{
			return { { BuildEventAllocator<Is>()... } };
		}

		inline static InternalAllocatorArray BuildInternalEventAllocators()
		{
			return BuildAllEventAllocators(std::make_index_sequence<EventList::All::Size>());
		}

		inline static InternalAllocatorMap ConstructAllocatorMap() 
		{
			InternalAllocatorMap result;
			for (auto&& [type, allocator] : BuildInternalEventAllocators())
			{
				result.emplace(type, std::move(allocator));
			}
			return result;
		}

	public:
		EventModelAllocator()
			: mModelAllocators(ConstructAllocatorMap())
		{

		}
		~EventModelAllocator()
		{
			CleanupDefaultNewPointers();
		}

		EventModelAllocator(const EventModelAllocator&) = delete;
		auto operator=(const EventModelAllocator&) = delete;

	public:
		/// <summary>
		/// Allocates and constructs a new event model for the event type T and returns a reference to it.
		/// Memory will be allocated from a pool allocator unless there is no more space in it this frame,
		/// in which case the memory will be allocated using default new operator. When Flush is called any
		/// of these default allocated pointers will be deleted and the pool allocator will be enlarged for
		/// the next frame.
		/// </summary>
		template<IsEvent T, typename... Args> requires std::constructible_from<T, Args...>
		EventModel<T>& NewModel(Args&&... args)
		{
			using EventType = TypeTraits<T>;

			if (!mModelAllocators.contains(EventType::Name))
				Register<T>();

			auto& model = mModelAllocators.at(EventType::Name);

			// If there is no more space in pool allocator, just use default allocator.
			// Pointer will be saved to be cleared up on flush at which point the pool 
			// will be enlarged. Don't do it here so we don't invalidate Events in queue.
			if (model.remaining == 0)
			{
				EventModel<T>* overflow = new EventModel<T>(std::forward<Args>(args)...);
				mOverflowPointers.push_back(overflow);
				return *overflow;
			}

			return ConstructModel<T>(model, std::forward<Args>(args)...);
		}

		/// <summary>
		/// Clean up any events allocated this frame, and enlarge any pool allocators that filled
		/// up this frame. Any default allocated pointers from this frame are also deleted here.
		/// </summary>
		void Flush()
		{
			for (auto& [type, model] : mModelAllocators)
			{
				model.allocator->Reset();

				// The allocator completely filled up during this frame. Reallocate larger to compensate.
				if (model.remaining == 0)
				{
					model.allocator->ForceReallocate();
				}

				model.remaining = model.allocator->MaxSize();
			}

			CleanupDefaultNewPointers();
		}

	private:
		template<IsEvent T>
		void Register()
		{
			using EventType = TypeTraits<T>;
			mModelAllocators.try_emplace(EventType::Name, MakeUnique<LinearAllocator<EventModel<T>>>(DefaultModelChunkSize));
		}

		void CleanupDefaultNewPointers()
		{
			for (EventConcept* ptr : mOverflowPointers)
			{
				delete ptr;
			}
			mOverflowPointers.clear();
		}

		template<IsEvent T, typename... Args> requires std::constructible_from<T, Args...>
		static EventModel<T>& ConstructModel(ModelAllocator& model, Args&&... args)
		{
			EventModel<T>* ptr = model.allocator->Alloc<EventModel<T>>(std::forward<Args>(args)...);
			model.remaining--;
			return *ptr;
		}

	private:
		InternalAllocatorMap mModelAllocators;
		std::vector<EventConcept*> mOverflowPointers;
	};
}
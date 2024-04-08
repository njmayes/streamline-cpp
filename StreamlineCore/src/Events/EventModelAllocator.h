#pragma once

#include "Event.h"

#include "Allocators/PoolAllocator.h"
#include "Containers/Dictionary.h"

namespace slc {

	/// <summary>
	/// Allocates and constructs event models for any given event type.
	/// </summary>
	class EventModelAllocator
	{
	private:
		struct ModelAllocator
		{
			Impl<IAllocator> allocator = nullptr;
			size_t remaining = 0;

			template<IsEvent T>
			ModelAllocator(Impl<PoolAllocator<EventModel<T>>> alloc)
				: allocator(std::move(alloc)), remaining(allocator->Size()) {}
		};

	public:
		EventModelAllocator() = default;
		~EventModelAllocator()
		{
			CleanupDefaultNewPointers();
		}

		EventModelAllocator(const EventModelAllocator&) = delete;
		EventModelAllocator(EventModelAllocator&&) = delete;
		auto operator=(const EventModelAllocator&) = delete;
		auto operator=(EventModelAllocator&&) = delete;

	public:
		/// <summary>
		/// Allocates and constructs a new event model for the event type T and returns a reference to it.
		/// Memory will be allocated from a pool allocator unless there is no more space in it this frame,
		/// in which case the memory will be allocated using default new operator.
		/// </summary>
		template<IsEvent T, typename... Args> requires std::constructible_from<T, Args...>
		EventModel<T>& NewModel(Args&&... args)
		{
			using EventType = TypeTraits<T>;

			if (!mModelAllocators.contains(EventType::LongName))
				Register<T>();

			auto& model = mModelAllocators.at(EventType::LongName);

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
				model.allocator->Free();

				// The allocator completely filled up during this frame. Reallocate larger to compensate.
				if (model.remaining == 0)
				{
					model.allocator->ForceReallocate();
				}

				model.remaining = model.allocator->Size();
			}

			CleanupDefaultNewPointers();
		}

	private:
		template<IsEvent T>
		void Register()
		{
			using EventType = TypeTraits<T>;
			mModelAllocators.try_emplace(EventType::LongName, MakeImpl<PoolAllocator<EventModel<T>>>(DefaultModelChunkSize));
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
		using TypeName = std::string_view;
		SCONSTEXPR size_t DefaultModelChunkSize = 4;

		Dictionary<TypeName, ModelAllocator> mModelAllocators;
		std::vector<EventConcept*> mOverflowPointers;
	};
}
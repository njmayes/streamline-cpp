#pragma once

#include <unordered_set>
#include <memory>

namespace slc {

	// Unique Pointer

	template<typename T>
	using Impl = std::unique_ptr<T>;

	template<typename T, typename... Args>
	inline static constexpr Impl<T> MakeImpl(Args&&... args) { return std::make_unique<T>(std::forward<Args>(args)...); }



	// Shared Pointer

	class RefCounted;

	template<typename T>
	concept RefCountable = std::derived_from<T, RefCounted>;

	namespace Internal {

		class RefCountedBase
		{
		public:
			uint64_t GetRefCount() const { return mRefCount; }

		protected:
			void IncRefCount() const { ++mRefCount; }
			void DecRefCount() const { --mRefCount; }

		private:
			mutable std::atomic_uint64_t mRefCount = 0;
		};
	}

	class RefCounted : public virtual Internal::RefCountedBase
	{
		template<RefCountable T>
		friend class Ref;
	};

	namespace detail {

		class RefTracker
		{
		public:
			static bool IsTracked(void* data);

		private:
			static void AddToReferenceTracker(void* data);
			static void RemoveFromReferenceTracker(void* data);

		private:
			using RefSet = std::unordered_set<void*>;
			inline static RefSet sRefSet;

			template<RefCountable T>
			friend class Ref;
		};
	}

	template<RefCountable T>
	class Ref
	{
	public:
		Ref() : mData(nullptr) {}
		Ref(std::nullptr_t) : mData(nullptr) {}
		Ref(T* data) : mData(data) { IncRef(); }

		template<typename Other>
		Ref(const Ref<Other>& other)
		{
			mData = static_cast<T*>(other.mData);
			IncRef();
		}

		template<typename Other>
		Ref(Ref<Other>&& other)
		{
			mData = static_cast<T*>(other.mData);
			other.mData = nullptr;
		}

		Ref(const Ref<T>& other)
			: mData(other.mData)
		{
			IncRef();
		}

		~Ref() { DecRef(); }

		Ref& operator=(std::nullptr_t)
		{
			DecRef();
			mData = nullptr;
			return *this;
		}

		Ref& operator=(const Ref<T>& other)
		{
			other.IncRef();
			DecRef();

			mData = static_cast<T*>(other.mData);
			return *this;
		}

		template<typename Other>
		Ref& operator=(const Ref<Other>& other)
		{
			other.IncRef();
			DecRef();

			mData = static_cast<T*>(other.mData);
			return *this;
		}

		template<typename Other>
		Ref& operator=(Ref<Other>&& other)
		{
			DecRef();

			mData = other.mData;
			other.mData = nullptr;
			return *this;
		}

		operator bool() const { return mData != nullptr; }

		T* operator->() { return mData; }
		const T* operator->() const { return mData; }

		T& operator*() { return *mData; }
		const T& operator*() const { return *mData; }

		T* Data() { return mData; }
		const T* Data() const { return mData; }

		bool operator==(const Ref<T>& other) const { return mData == other.mData; }
		bool operator==(std::nullptr_t) const { return mData == nullptr; }

		void Reset()
		{
			DecRef();
			mData = nullptr;
		}

		template<typename Other>
		Ref<Other> To() const
		{
			return Ref<Other>(*this);
		}

		template<typename... Args>
		static Ref<T> Create(Args&&... args)
		{
			return Ref<T>(new T(std::forward<Args>(args)...));
		}

	private:
		void IncRef() const
		{
			if (!mData)
				return;

			mData->IncRefCount();
			detail::RefTracker::AddToReferenceTracker(static_cast<void*>(mData));
		}

		void DecRef()
		{
			if (!mData)
				return;

			mData->DecRefCount();
			if (mData->GetRefCount() == 0)
			{
				delete mData;
				detail::RefTracker::RemoveFromReferenceTracker(static_cast<void*>(mData));
				mData = nullptr;
			}
		}

	private:
		template<RefCountable Other>
		friend class Ref;

		T* mData;
	};


	// Weak Pointer

	template<RefCountable T>
	class WeakRef
	{
	public:
		WeakRef() = default;

		WeakRef(Ref<T> ref)
		{
			mData = ref.Data();
		}

		WeakRef(T* instance)
		{
			mData = instance;
		}

		T* operator->() { return mData; }
		const T* operator->() const { return mData; }

		T& operator*() { return *mData; }
		const T& operator*() const { return *mData; }

		bool Valid() const { return mData ? detail::RefTracker::IsTracked(mData) : false; }
		operator bool() const { return Valid(); }

		Ref<T> Lock() const
		{
			return Ref<T>(mData);
		}

	private:
		T* mData = nullptr;
	};
}

namespace std {
	template<typename T> struct hash;

	template<typename T>
	struct hash<slc::Ref<T>>
	{
		std::size_t operator()(const slc::Ref<T>& ref) const
		{
			return std::hash<const void*>()((const void*)ref.Data());
		}
	};

}
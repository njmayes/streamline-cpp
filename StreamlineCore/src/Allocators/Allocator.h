#pragma once

namespace slc {

	/// <summary>
	/// Base allocator interface.
	/// Inherited classes must provide overrides to retrieve and free fixed size blocks of memory, as well as a total size override.
	/// </summary>
	class IAllocator
	{
	public:
		virtual ~IAllocator() {}

		template<typename T,typename... Args>
		T* Alloc(Args&&... args)
		{
			T* ptr = static_cast<T*>(Alloc(sizeof(T)));
			new (ptr) T(std::forward<Args>(args)...);
			return ptr;
		}

		virtual void Free(void* ptr = nullptr) = 0;
		virtual size_t Size() const = 0;

		virtual void ForceReallocate() {}

	protected:
		virtual void* Alloc(size_t size) = 0;
	};
}
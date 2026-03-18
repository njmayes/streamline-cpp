#pragma once

#include <cstddef>
#include <atomic>

namespace sl {

	class RefCounted;

	namespace detail {

		class RefCountedBase
		{
			friend RefCounted;

		public:
			std::uint64_t GetRefCount() const
			{
				return mRefCount;
			}

		protected:
			RefCountedBase() = default;
			~RefCountedBase() = default;

			void IncRefCount() const
			{
				++mRefCount;
			}

			void DecRefCount() const
			{
				--mRefCount;
			}

		private:
			mutable std::atomic_uint64_t mRefCount = 0;
		};
	} // namespace detail
}
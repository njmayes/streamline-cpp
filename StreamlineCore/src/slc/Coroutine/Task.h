#pragma once

#include "Internal/Task.h"

namespace slc {

	template<typename TReturn>
	class [[nodiscard]] Task
	{
	public:
		// Coroutine interface
		using promise_type = detail::TaskPromise<TReturn>;

	public:
		using CoroutineHandle = std::coroutine_handle<promise_type>;

		using CopyAwaitable = detail::CopyTaskAwaitable<TReturn>;
		using MoveAwaitable = detail::MoveTaskAwaitable<TReturn>;

	public:
		Task() noexcept : mHandle(nullptr) {}
		explicit Task(CoroutineHandle handle) : mHandle(handle) {}

		Task(const Task&) = delete;
		Task(Task&& other) noexcept : mHandle(std::exchange(other.mHandle, nullptr)) {}

		~Task()
		{
			if (mHandle)
				mHandle.destroy();
		}

		auto operator=(const Task&) = delete;
		auto operator=(Task&& other)
		{
			if (std::addressof(other) != this)
			{
				if (mHandle)
					mHandle.destroy();

				mHandle = std::exchange(other.mHandle, nullptr);
			}
			
			return *this;
		}

	public:
		auto operator co_await() const& noexcept { return CopyAwaitable{ mHandle }; }
		auto operator co_await() const&& noexcept { return MoveAwaitable{ mHandle }; }

	private:
		CoroutineHandle mHandle;
	};

	template<typename TReturn>
	auto detail::TaskPromise<TReturn>::get_return_object() noexcept -> Task<TReturn>
	{
		return Task<TReturn> { Task<TReturn>::CoroutineHandle::from_promise(*this) };
	}

	auto detail::TaskPromise<void>::get_return_object() noexcept -> Task<>
	{
		return Task<> { Task<>::CoroutineHandle::from_promise(*this) };
	}

}
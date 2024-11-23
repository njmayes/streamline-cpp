#pragma once

#include "slc/Common/Base.h"

#include <coroutine>

namespace slc {

	template<typename TReturn = void>
	class Task;
}

namespace slc::detail {

	struct TaskPromiseAwaiter
	{
		auto await_ready() const noexcept { return false; }

		template<typename TPromise>
		auto await_suspend(std::coroutine_handle<TPromise> coroutine) const noexcept -> std::coroutine_handle<>
		{
			auto& promise = coroutine.promise();
			if (promise.mHandle)
			{
				// Resuming, return continuation
				return promise.mHandle;
			}
			else
			{
				return std::noop_coroutine();
			}
		}
		auto await_resume() noexcept {};
	};

	class TaskPromiseBase
	{
	public:
		TaskPromiseBase() noexcept = default;
		~TaskPromiseBase() = default;

		auto initial_suspend() noexcept { return std::suspend_never{}; }
		auto final_suspend() noexcept { return TaskPromiseAwaiter{}; }

		auto set(std::coroutine_handle<> handle) noexcept -> void { mHandle = handle; }

	protected:
		friend struct TaskPromiseAwaiter;

		std::coroutine_handle<> mHandle;
	};

	template<typename TReturn>
	class TaskPromise final : public TaskPromiseBase
	{
	private:
		struct InvalidPromiseState
		{
			InvalidPromiseState() {}
			InvalidPromiseState(InvalidPromiseState&&) = delete;
			InvalidPromiseState(const InvalidPromiseState&) = delete;
			auto operator=(InvalidPromiseState&&) = delete;
			auto operator=(const InvalidPromiseState&) = delete;
		};

	public:
		using CoroutineType = std::coroutine_handle<TaskPromise<TReturn>>;
		using TaskType = Task<TReturn>;

		static constexpr bool IsReturnReferenceType = std::is_reference_v<TReturn>;
		using ResultType = std::conditional_t<
			IsReturnReferenceType,
			std::remove_reference_t<TReturn>*,
			std::remove_const_t<TReturn>
		>;

		using StorageType = std::variant<InvalidPromiseState, ResultType, std::exception_ptr>;

	public:
		auto get_return_object() noexcept -> TaskType;

		template<typename TValue> requires
			( IsReturnReferenceType && std::is_constructible_v<TReturn,	 TValue&&>) or
			(!IsReturnReferenceType && std::is_constructible_v<ResultType, TValue&&>)
		auto return_value(TValue&& value) -> void
		{
			if constexpr (IsReturnReferenceType)
			{
				TReturn ref = static_cast<TValue&&>(value);
				mResult.emplace<ResultType>(std::addressof(ref));
			}
			else
			{
				mResult.emplace<ResultType>(std::forward<TValue>(value));
			}
		}

		auto return_value(ResultType value) -> void requires(!IsReturnReferenceType)
		{
			if constexpr (std::is_move_constructible_v<ResultType>)
			{
				mResult.emplace<ResultType>(std::move(value));
			}
			else
			{
				mResult.emplace<ResultType>(value);
			}
		}

		auto unhandled_exception() noexcept -> void { mResult.emplace<std::exception_ptr>(std::current_exception()); }

		auto extract_result() & -> decltype(auto)
		{
			if (std::holds_alternative<ResultType>(mResult))
			{
				if constexpr (IsReturnReferenceType)
				{
					return static_cast<TReturn>(*std::get<ResultType>(mResult));
				}
				else
				{
					return static_cast<const TReturn&>(std::get<ResultType>(mResult));
				}
			}
			else if (std::holds_alternative<std::exception_ptr>(mResult))
			{
				std::rethrow_exception(std::get<std::exception_ptr>(mResult));
			}
			else
			{
				throw std::runtime_error("Could not extract return value from coroutine, did the coroutine run?");
			}
		}
		auto extract_result() const& -> decltype(auto)
		{
			if (std::holds_alternative<ResultType>(mResult))
			{
				if constexpr (IsReturnReferenceType)
				{
					return static_cast<std::add_const_t<TReturn>>(*std::get<ResultType>(mResult));
				}
				else
				{
					return static_cast<const TReturn&>(std::get<ResultType>(mResult));
				}
			}
			else if (std::holds_alternative<std::exception_ptr>(mResult))
			{
				std::rethrow_exception(std::get<std::exception_ptr>(mResult));
			}
			else
			{
				throw std::runtime_error("Could not extract return value from coroutine, did the coroutine run?");
			}
		}
		auto extract_result() && -> decltype(auto)
		{
			if (std::holds_alternative<ResultType>(mResult))
			{
				if constexpr (IsReturnReferenceType)
				{
					return static_cast<TReturn>(*std::get<ResultType>(mResult));
				}
				else
				{
					return static_cast<TReturn&&>(std::get<ResultType>(mResult));
				}
			}
			else if (std::holds_alternative<std::exception_ptr>(mResult))
			{
				std::rethrow_exception(std::get<std::exception_ptr>(mResult));
			}
			else
			{
				throw std::runtime_error("Could not extract return value from coroutine, did the coroutine run?");
			}
		}

	private:
		template<typename TReturn>
		friend struct TaskAwaitableBase;

		StorageType mResult;
	};

	template<>
	class TaskPromise<void> : public TaskPromiseBase
	{
	public:
		using CoroutineType = std::coroutine_handle<TaskPromise<void>>;
		using TaskType = Task<void>;

		auto get_return_object() noexcept -> TaskType;

		auto return_void() noexcept -> void {}
		auto unhandled_exception() noexcept -> void { mException = std::current_exception(); }

		auto extract_result() -> void
		{
			if (mException)
			{
				std::rethrow_exception(mException);
			}
		}

	private:
		template<typename TReturn>
		friend struct TaskAwaitableBase;

		std::exception_ptr mException;
	};

	template<typename TReturn>
	struct TaskAwaitableBase
	{
		using CoroutineHandle = std::coroutine_handle<TaskPromise<TReturn>>;

		auto await_ready() const noexcept 
		{ 
			return !handle || handle.done(); 
		}
		auto await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept -> std::coroutine_handle<>
		{
			handle.promise().set(awaitingCoroutine);
			return handle;
		}

		CoroutineHandle handle;
	};

	template<typename TReturn>
	struct CopyTaskAwaitable : public TaskAwaitableBase<TReturn>
	{
		auto await_resume() -> decltype(auto) 
		{
			return this->handle.promise().extract_result();
		}
	};
	template<typename TReturn>
	struct MoveTaskAwaitable : public TaskAwaitableBase<TReturn>
	{
		auto await_resume() -> decltype(auto) 
		{
			if constexpr (!std::is_void_v<TReturn>)
				return std::move(this->handle.promise().extract_result());
			else
				this->handle.promise().extract_result();
		}
	};
}
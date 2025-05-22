#pragma once

#include "Result.h"

namespace slc {

	namespace detail {
		SLC_MAKE_RUST_ENUM( NoneEnum, None )
	}

	template<typename T>
	class Option : public Result<T, detail::NoneEnum>
	{
	public:
		SCONSTEXPR detail::NoneEnum None = detail::NoneEnum::None;

	private:
		using BaseType = Result<T, detail::NoneEnum>;

	public:
		template<IsEnum E>
		Result<T, E> ok_or(E err) noexcept(BaseType::IsNoExceptMove)
		{
			using ReturnType = Result<T, E>;
			return this->is_ok() ? ReturnType(this->GetVal()) : ReturnType(err);
		}
		template<IsEnum E>
		Result<T, E> ok_or_else(IsFunc<E> auto&& err) noexcept(BaseType::IsNoExceptMove)
		{
			using ReturnType = Result<T, E>;
			return this->is_ok() ? ReturnType(this->GetVal()) : ReturnType(err);
		}
	};

	template<typename T>
	struct SomeFunctor;

	template<typename T>
	struct NoneFunctor;

	template<typename T>
	struct SomeFunctor<Option<T>>
	{
		constexpr Option<T> operator()(T&& result) const noexcept(noexcept(T(std::forward<T>(std::declval<T>()))))
		{
			return Option<T>(std::forward<T>(result));
		}

		template<typename... Args>
		constexpr Option<T> operator()(Args&&... args) const noexcept(Option<T>::IsNoExceptMove && Option<T>::template IsNoExceptNew<Args...>)
		{
			T&& val = T(std::forward<Args>(args)...);
			return Option<T>(std::move(val));
		}
	};

	template<typename T>
	struct NoneFunctor<Option<T>>
	{
		constexpr Option<T> operator()() const noexcept
		{
			return Option<T>(Option<T>::None);
		}
	};

	template<typename T>
	SCONSTEXPR SomeFunctor<T> Some;

	template<typename T>
	SCONSTEXPR NoneFunctor<T> None;
}
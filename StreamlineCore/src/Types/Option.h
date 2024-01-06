#pragma once

#include "Result.h"

namespace slc {

	namespace OptionEnum {

		enum NoneEnum { None };
	}

	template<typename T>
	class Option : public Result<T, OptionEnum::NoneEnum>
	{
	private:
		using BaseType = Result<T, OptionEnum::NoneEnum>;

	public:
		template<IsEnum E>
		Result<T, E> ok_or(E err) noexcept(BaseType::NoExceptMove)
		{
			using ReturnType = Result<T, E>;
			return this->mResult ? ReturnType(this->GetVal()) : ReturnType(err);
		}
		template<IsEnum E>
		Result<T, E> ok_or_else(IsFunc<E> auto&& err) noexcept(BaseType::NoExceptMove)
		{
			using ReturnType = Result<T, E>;
			return this->mResult ? ReturnType(this->GetVal()) : ReturnType(err);
		}
	};

	template<typename T>
	struct SomeFunctor;

	template<typename T>
	struct NoneFunctor;

	template<typename T>
	struct SomeFunctor<Option<T>>
	{
		constexpr Option<T> operator()(T&& result) const noexcept(Option<T>::NoExceptMove)
		{
			return Option<T>(std::move(result));
		}

		template<typename... Args>
		constexpr Option<T> operator()(Args&&... args) const noexcept(Option<T>::NoExceptNew)
		{
			return Option<T>(std::move(T(std::forward<Args>(args)...)));
		}
	};

	template<typename T, IsEnum E>
	struct NoneFunctor<Result<T, E>>
	{
		constexpr Result<T, E> operator()() const noexcept
		{
			return Option<T>(OptionEnum::None);
		}
	};

	template<typename T>
	SCONSTEXPR SomeFunctor<T> Some;

	template<typename T>
	SCONSTEXPR NoneFunctor<T> None;
}
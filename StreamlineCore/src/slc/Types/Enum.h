#pragma once

#include "slc/Common/Base.h"

#define SLC_PROBE(x) x, 1
#define SLC_IS_PROBE(...) SLC_GET_SECOND(__VA_ARGS__, 0)
#define SLC_GET_SECOND(a, b, ...) b

#define SLC_IS_PAREN(x) SLC_IS_PROBE(SLC_CHECK_PAREN x)
#define SLC_CHECK_PAREN(...) SLC_PROBE(~)

#define SLC_EXTRACT_IDENT(x) SLC_EXTRACT_IDENT_IMPL(SLC_IS_PAREN(x), x)
#define SLC_EXTRACT_IDENT_IMPL(is_paren, x) SLC_EXTRACT_IDENT_SELECT(is_paren, x)
#define SLC_EXTRACT_IDENT_SELECT(is_paren, x) SLC_EXTRACT_IDENT_##is_paren(x)

#define SLC_EXTRACT_IDENT_0(x) x
#define SLC_EXTRACT_IDENT_1(x) SLC_EXTRACT_IDENT_FIRST x
#define SLC_EXTRACT_IDENT_FIRST(a, b) a

#define SLC_EXTRACT_TYPE(x) SLC_EXTRACT_TYPE_IMPL(SLC_IS_PAREN(x), x)
#define SLC_EXTRACT_TYPE_IMPL(is_paren, x) SLC_EXTRACT_TYPE_SELECT(is_paren, x)
#define SLC_EXTRACT_TYPE_SELECT(is_paren, x) SLC_EXTRACT_TYPE_##is_paren(x)

#define SLC_EXTRACT_TYPE_0(x) std::monostate
#define SLC_EXTRACT_TYPE_1(x) SLC_EXTRACT_TYPE_SECOND x

#define SLC_EXTRACT_TYPE_SECOND(a, b) b

#define SLC_MATCH_CASE(enum_case)																	\
	case enum_case:																					\
	{																								\
		invoker.template operator()<enum_case>();													\
		break;																						\
	}																								


namespace slc {

	template<typename... Ts>
	concept ValidEnumTypes = ((not std::is_pointer_v<Ts> and std::same_as<std::remove_cvref_t<Ts>, Ts>) and ...);
}

#define SLC_MAKE_RUST_ENUM(name, ...)																																				\
namespace detail_##name {																																							\
	template<typename T>																																							\
	class name##_RustEnum;																																							\
																																													\
	template<typename... Ts> requires ::slc::ValidEnumTypes<Ts...>																													\
	class name##_RustEnum<::slc::TypeList<Ts...>>																																	\
	{																																												\
	public:																																											\
		enum name##_UnderlyingEnum : std::size_t																																	\
		{																																											\
			SLC_FOR_EACH_SEP(SLC_EXTRACT_IDENT, SLC_COMMA, __VA_ARGS__)																												\
		};																																											\
																																													\
		using Enum = name##_UnderlyingEnum;																																			\
																																													\
		template<Enum Element>																																						\
		using EnumConstant = std::integral_constant<Enum, Element>;																													\
																																													\
		template<typename T>																																						\
		using MatchFunction = std::conditional_t<std::same_as<T, std::monostate>, std::function<void()>, std::function<void(T const&)>>;											\
																																													\
	private:																																										\
		using ValueTypes = ::slc::TypeList<Ts...>;																																	\
		using FunctionTypes = ::slc::TypeList<MatchFunction<Ts>...>;																												\
		using Self = name##_RustEnum<ValueTypes>;																																	\
		using ValueStorageType = ValueTypes::VariantType;																															\
		using FunctionStorageType = FunctionTypes::TupleType;																														\
																																													\
		template<Enum Element>																																						\
		using ValueTypeAt = typename ValueTypes::template Type<Element>;																											\
																																													\
		template<Enum Element>																																						\
		using FunctionTypeAt = typename FunctionTypes::template Type<Element>;																										\
																																													\
		template<Enum Element>																																						\
		SCONSTEXPR bool HasType = !(std::same_as<ValueTypeAt<Element>, std::monostate>);																							\
																																													\
		template<typename T>																																						\
		SCONSTEXPR bool ContainsType = ValueTypes::template Contains<T>;																											\
																																													\
	public:																																											\
		name##_RustEnum() = default;																																				\
		name##_RustEnum(name##_RustEnum const&) = default;																															\
		name##_RustEnum(name##_RustEnum&&) = default;																																\
		name##_RustEnum& operator=(name##_RustEnum const&) = default;																												\
		name##_RustEnum& operator=(name##_RustEnum&&) = default;																													\
		~name##_RustEnum() = default;																																				\
																																													\
		template<typename T> requires ContainsType<T>																																\
		name##_RustEnum(T&& value)																																					\
			: mValueData(std::forward<T>(value))																																	\
		{																																											\
		}																																											\
		template<Enum Element> requires std::is_default_constructible_v<ValueTypeAt<Element>>																						\
		name##_RustEnum(EnumConstant<Element>)																																		\
			: mValueData(std::in_place_index_t<Element>{})																															\
		{																																											\
		}																																											\
																																													\
		template<typename T> requires ContainsType<T>																																\
		name##_RustEnum& operator=(T&& value)																																		\
		{																																											\
			mValueData = std::forward<T>(value);																																	\
			return *this;																																							\
		}																																											\
		template<Enum Element> requires std::is_default_constructible_v<ValueTypeAt<Element>>																						\
		name##_RustEnum& operator=(EnumConstant<Element>)																															\
		{																																											\
			mValueData.emplace<Element>();																																			\
			return *this;																																							\
		}																																											\
																																													\
		template<Enum Element, typename Func> requires std::convertible_to<Func, FunctionTypeAt<Element>>																			\
		Self& SetMatchFunc(Func&& func)																																				\
		{																																											\
			std::get<Element>(mMatchData) = std::move(func);																														\
			return *this;																																							\
		}																																											\
																																													\
		void Match()																																								\
		{																																											\
			auto invoker = [this]<Enum Element>(){																																	\
				if constexpr (HasType<Element>)																																		\
					std::invoke(std::get<Element>(mMatchData), *std::get_if<Element>(&mValueData));																					\
				else																																								\
					std::invoke(std::get<Element>(mMatchData));																														\
			};																																										\
																																													\
			switch (mValueData.index())																																				\
			{																																										\
				SLC_FOR_EACH(SLC_MATCH_CASE, SLC_FOR_EACH_SEP(SLC_EXTRACT_IDENT, SLC_COMMA, __VA_ARGS__))																			\
			}																																										\
		}																																											\
																																													\
	private:																																										\
		template<Enum Element>																																						\
		bool IsHolding() const noexcept																																				\
		{																																											\
			return std::get_if<Element>(&mValueData) != nullptr;																													\
		}																																											\
																																													\
		template<Enum Element>																																						\
		ValueTypeAt<Element> const& Get() noexcept																																	\
		{																																											\
			return *std::get_if<Element>(&mValueData);																																\
		}																																											\
																																													\
	private:																																										\
		ValueStorageType mValueData;																																				\
		FunctionStorageType mMatchData;																																				\
	};																																												\
																																													\
	using Impl = name##_RustEnum<::slc::TypeList<SLC_FOR_EACH_SEP(SLC_EXTRACT_TYPE, SLC_COMMA, __VA_ARGS__)>>;																		\
}																																													\
using name = detail_##name::Impl;																						

/*
	MAKE_RUST_ENUM(Error, 
		OutOfBounds,
		Unexpected(std::string)
	)

	Error test = Error::OutOfBounds;

	#define MAKE_RUST_ENUM_TYPE(type)	\



	#define MAKE_RUST_ENUM(name, ...)							\
		namespace detail {										\
			enum class name##Enum								\
			{													\
			}													\
		}														\
		using name = RustEnum<__VA_ARGS__>;
*/
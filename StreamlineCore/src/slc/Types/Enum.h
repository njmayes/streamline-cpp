#pragma once

#include "slc/Common/Base.h"

namespace slc {

	template<typename... Ts>
	concept ValidEnumTypes = ((not std::is_pointer_v<Ts> and std::same_as<std::remove_cvref_t<Ts>, Ts>) and ...);

	template<typename... Ts> requires ValidEnumTypes<Ts...>
	class RustEnum
	{
	private:
		using Types = TypeList<Ts...>;
		using StorageType = Types::VariantType;

	private:
		StorageType mData;
	};
}


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



#define SLC_MAKE_RUST_ENUM_UNDERLYING(name, ...)		\
namespace detail_##name {								\
	using Types = ::slc::TypeList<SLC_FOR_EACH_SEP(SLC_EXTRACT_TYPE, SLC_COMMA, __VA_ARGS__ )>;	\
																				\
	enum class name##_UnderlyingEnum : std::size_t		\
	{													\
		SLC_FOR_EACH_SEP(SLC_EXTRACT_IDENT, SLC_COMMA, __VA_ARGS__)	\
	};													\
}

SLC_MAKE_RUST_ENUM_UNDERLYING(Test,
	OutOfBounds,
	(Unexpected, std::string)
)


namespace detail_Test {

	template<typename T>
	class Test_RustEnum;

	template<typename... Ts>
	struct Test_RustEnum<::slc::TypeList<Ts...>>
	{
	private:
		using Types = ::slc::TypeList<Ts...>;
		using StorageType = Types::VariantType;

	public:
		enum Test_UnderlyingEnum : std::size_t
		{
			OutOfBounds,
			Unexpected
		};

		template<Test_UnderlyingEnum Element>
		using TypeAt = typename Types::template Type<Element>;

		template<Test_UnderlyingEnum Element>
		bool IsHolding() const noexcept
		{
			return std::get_if<Element>(&mData) != nullptr;
		}

		template<Test_UnderlyingEnum Element>
		TypeAt<Element> const& Get()
		{
			return *std::get_if<Element>(&mData);
		}

	private:
		StorageType mData;
	};

	using Types = ::slc::TypeList<std::monostate, std::string>;
	using Impl = Test_RustEnum<Types>;
}

using Test = detail_Test::Impl;

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
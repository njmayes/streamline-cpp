#pragma once

#include "slc/Common/Base.h"

#define SLC_PROBE( x ) x, 1
#define SLC_IS_PROBE( ... ) SLC_GET_SECOND( __VA_ARGS__, 0 )
#define SLC_GET_SECOND( a, b, ... ) b

#define SLC_IS_PAREN( x ) SLC_IS_PROBE( SLC_CHECK_PAREN x )
#define SLC_CHECK_PAREN( ... ) SLC_PROBE( ~)

#define SLC_EXTRACT_IDENT( x ) SLC_EXTRACT_IDENT_IMPL( SLC_IS_PAREN( x ), x )
#define SLC_EXTRACT_IDENT_IMPL( is_paren, x ) SLC_EXTRACT_IDENT_SELECT( is_paren, x )
#define SLC_EXTRACT_IDENT_SELECT( is_paren, x ) SLC_EXTRACT_IDENT_##is_paren( x )

#define SLC_EXTRACT_IDENT_0( x ) x
#define SLC_EXTRACT_IDENT_1( x ) SLC_EXTRACT_IDENT_FIRST x
#define SLC_EXTRACT_IDENT_FIRST( a, b ) a

#define SLC_EXTRACT_TYPE( x ) SLC_EXTRACT_TYPE_IMPL( SLC_IS_PAREN( x ), x )
#define SLC_EXTRACT_TYPE_IMPL( is_paren, x ) SLC_EXTRACT_TYPE_SELECT( is_paren, x )
#define SLC_EXTRACT_TYPE_SELECT( is_paren, x ) SLC_EXTRACT_TYPE_##is_paren( x )

#define SLC_EXTRACT_TYPE_0( x ) std::monostate
#define SLC_EXTRACT_TYPE_1( x ) SLC_EXTRACT_TYPE_SECOND x

#define SLC_EXTRACT_TYPE_SECOND( a, b ) b

#define SLC_MATCH_CASE( enum_case )																\
	case std::to_underlying( Enum::enum_case ):													\
	{																							\
		if constexpr ( HasType< Enum::enum_case > )												\
			std::forward< Matcher >( matcher )( enum_case, GetValue< Enum::enum_case >() );		\
		else																					\
			std::forward< Matcher >( matcher )( enum_case );									\
		break;																					\
	}

#define SLC_MAKE_INTERNAL_ENUM( enum_case ) \
	SCONSTEXPR auto enum_case = ::slc::detail::EnumTag< Enum, Enum::enum_case >{};

namespace slc
{
	namespace detail
	{
		template < typename... Ts >
		concept ValidEnumTypes = ( ( not std::is_pointer_v< Ts > and std::same_as< std::remove_cvref_t< Ts >, Ts > ) and ... );

		struct RustEnum_Base
		{};


		template < typename Enum, Enum E > requires std::is_scoped_enum_v< Enum >
		struct EnumTag : std::integral_constant< Enum, E >
		{
			SCONSTEXPR auto Value = std::to_underlying( E );
		};

		template < auto E, typename F >
		struct MatchCaseHandler
		{
			F func;

			template < typename... Args >
			auto operator()( decltype(E), Args&&... args ) const
			{
				return func( std::forward< Args >( args )... );
			}
		};


		template < typename... Fs >
		struct Overload : Fs...
		{
			using Fs::operator()...;
		};
		template < typename... Fs >
		Overload( Fs... ) -> Overload< Fs... >;

	}

	template<typename T>
	concept IsRustEnum = std::derived_from< T, detail::RustEnum_Base >;


	template < auto E, typename F >
	detail::MatchCaseHandler< E, F > MatchCase( F&& f )
	{
		return detail::MatchCaseHandler< E, F >{ std::forward< F >( f ) };
	}
}

#define SLC_MAKE_RUST_ENUM( name, ... )                                                                                                                \
	namespace detail_##name                                                                                                                            \
	{                                                                                                                                                  \
		template < typename T >                                                                                                                        \
		class name##_RustEnum;																														   \
                                                                                                                                                       \
		template < typename... Ts >                                                                                                                    \
			requires ::slc::detail::ValidEnumTypes< Ts... >                                                                                            \
		class name##_RustEnum< ::slc::TypeList< Ts... > > : ::slc::detail::RustEnum_Base															   \
		{                                                                                                                                              \
		private:                                                                                                                                       \
			enum class name##_UnderlyingEnum : std::size_t{																							   \
				SLC_FOR_EACH_SEP( SLC_EXTRACT_IDENT, SLC_COMMA, __VA_ARGS__ )                                                                          \
			};                                                                                                                                         \
                                                                                                                                                       \
			using Enum = name##_UnderlyingEnum;                                                                                                        \
																																					   \
		public:																																		   \
			SLC_FOR_EACH( SLC_MAKE_INTERNAL_ENUM, SLC_FOR_EACH_SEP( SLC_EXTRACT_IDENT, SLC_COMMA, __VA_ARGS__ ) ) 									   \
                                                                                                                                                       \
		private:                                                                                                                                       \
			using ValueTypes = ::slc::TypeList< Ts... >;                                                                                               \
			using Self = name##_RustEnum< ValueTypes >;                                                                                                \
			using ValueStorageType = ValueTypes::VariantType;                                                                                          \
                                                                                                                                                       \
			template < Enum Element >																												   \
			using ValueTypeAt = typename ValueTypes::template Type< ::slc::detail::EnumTag< Enum, Element >::Value >;								   \
                                                                                                                                                       \
			template < Enum Element >                                                                                                                  \
			SCONSTEXPR bool HasType = !( std::same_as< ValueTypeAt< Element >, std::monostate > );                                                     \
                                                                                                                                                       \
			template < typename T >                                                                                                                    \
			SCONSTEXPR bool ContainsType = ValueTypes::template Contains< T >;                                                                         \
                                                                                                                                                       \
		public:                                                                                                                                        \
			name##_RustEnum() = default;                                                                                                               \
			name##_RustEnum( name##_RustEnum const& ) = default;                                                                                       \
			name##_RustEnum( name##_RustEnum&& ) = default;                                                                                            \
			name##_RustEnum& operator=( name##_RustEnum const& other )                                                                                 \
			{                                                                                                                                          \
				mValueData = other.mValueData;                                                                                                         \
				return *this;                                                                                                                          \
			}                                                                                                                                          \
			name##_RustEnum& operator=( name##_RustEnum&& other ) noexcept                                                                             \
			{                                                                                                                                          \
				mValueData = std::move( other.mValueData );                                                                                            \
				return *this;                                                                                                                          \
			}                                                                                                                                          \
			~name##_RustEnum() = default;                                                                                                              \
                                                                                                                                                       \
			template < Enum Element, typename... Args >																								   \
			name##_RustEnum( ::slc::detail::EnumTag< Enum, Element >, Args&&... args )																   \
				: mValueData( std::in_place_index_t< ::slc::detail::EnumTag< Enum, Element >::Value >{}, std::forward< Args >( args )... )			   \
			{																																		   \
			}																																		   \
                                                                                                                                                       \
			template < typename... Cases >																											   \
			void Match( Cases&&... cases )																											   \
			{																																		   \
				auto matcher = ::slc::detail::Overload{ std::forward< Cases >( cases )... };														   \
				using Matcher = decltype( matcher );																								   \
																																					   \
				switch ( mValueData.index() )                                                                                                          \
				{                                                                                                                                      \
					SLC_FOR_EACH( SLC_MATCH_CASE, SLC_FOR_EACH_SEP( SLC_EXTRACT_IDENT, SLC_COMMA, __VA_ARGS__ ) )                                      \
				}                                                                                                                                      \
			}                                                                                                                                          \
                                                                                                                                                       \
		private:                                                                                                                                       \
			template < Enum Element >                                                                                                                  \
			ValueTypeAt< Element > const& GetValue() noexcept                                                                                          \
			{                                                                                                                                          \
				return *std::get_if< ::slc::detail::EnumTag< Enum, Element >::Value >( &mValueData );                                                  \
			}                                                                                                                                          \
                                                                                                                                                       \
		private:                                                                                                                                       \
			ValueStorageType mValueData;                                                                                                               \
		};                                                                                                                                             \
                                                                                                                                                       \
		using Impl = name##_RustEnum< ::slc::TypeList< SLC_FOR_EACH_SEP( SLC_EXTRACT_TYPE, SLC_COMMA, __VA_ARGS__ ) > >;                               \
	}                                                                                                                                                  \
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
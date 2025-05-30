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

#define SLC_MATCH_CASE( enum_case )                                            \
	case std::to_underlying( Enum::enum_case ):                                \
		DispatchCase< Enum::enum_case >( std::forward< Matcher >( matcher ) ); \
		break;


#define SLC_MAKE_INTERNAL_ENUM( enum_case ) \
	SCONSTEXPR auto enum_case = ::slc::detail::EnumTag< Enum::enum_case >{};

namespace slc
{
	namespace detail
	{
		template < typename... Ts >
		concept ValidEnumTypes = ( ( not std::is_pointer_v< Ts > and std::same_as< std::remove_cvref_t< Ts >, Ts > ) and ... );

		struct RustEnum_Base
		{};


		template < auto E >
			requires std::is_scoped_enum_v< decltype( E ) >
		struct EnumTag : std::integral_constant< decltype( E ), E >
		{
			SCONSTEXPR auto Value = std::to_underlying( E );
		};

		template < auto E, typename F >
		struct EnumMatchCaseHandler
		{
			F func;

			template < typename... Args >
			auto operator()( decltype( E ), Args&&... args ) const
			{
				return func( std::forward< Args >( args )... );
			}
		};

		template < typename F >
		struct EnumDefaultMatchCaseHandler
		{
			F func;

			template < typename... Args >
			auto operator()( std::monostate, Args&&... args ) const
			{
				return func( std::forward< Args >( args )... );
			}
		};

		template < typename... Fs >
		struct Overload : Fs...
		{
			using Fs::operator()...;
		};
		template < typename... Ts >
		Overload( Ts... ) -> Overload< Ts... >;


		template < typename T >
		struct IsDefaultMatchCaseHandler : std::false_type
		{};

		template < typename F >
		struct IsDefaultMatchCaseHandler< EnumDefaultMatchCaseHandler< F > > : std::true_type
		{};

		template < typename... Ts >
		struct FindDefaultHandlerHelper
		{
			using type = void; // Sentinel indicating "not found"
		};

		template < typename T, typename... Rest >
		struct FindDefaultHandlerHelper< T, Rest... >
		{
			using type = std::conditional_t<
				IsDefaultMatchCaseHandler< T >::value,
				T,
				typename FindDefaultHandlerHelper< Rest... >::type >;
		};


		template < typename T >
		struct ExtractDefaultHandler;

		template < template < typename... > class OverloadT, typename... Cases >
		struct ExtractDefaultHandler< OverloadT< Cases... > >
		{
			using type = typename FindDefaultHandlerHelper< Cases... >::type;
		};

	} // namespace detail

	template < typename T >
	concept IsSmartEnum = std::derived_from< T, detail::RustEnum_Base >;


	template < auto E, typename F >
	detail::EnumMatchCaseHandler< E, F > MatchCase( F&& f )
	{
		return detail::EnumMatchCaseHandler< E, F >{ std::forward< F >( f ) };
	}

	template < typename F >
	detail::EnumDefaultMatchCaseHandler< F > DefaultCase( F&& f )
	{
		return detail::EnumDefaultMatchCaseHandler< F >{ std::forward< F >( f ) };
	}
} // namespace slc

#define SLC_MAKE_SMART_ENUM( name, ... )                                                                                                          \
	struct detail_##name                                                                                                                          \
	{                                                                                                                                             \
		template < typename T >                                                                                                                   \
		class name##_RustEnum;                                                                                                                    \
                                                                                                                                                  \
		template < typename... Ts >                                                                                                               \
			requires ::slc::detail::ValidEnumTypes< Ts... >                                                                                       \
		class name##_RustEnum< ::slc::TypeList< Ts... > > : public ::slc::detail::RustEnum_Base                                                   \
		{                                                                                                                                         \
		private:                                                                                                                                  \
			enum class Enum : std::size_t                                                                                                         \
			{                                                                                                                                     \
				SLC_FOR_EACH_SEP( SLC_EXTRACT_IDENT, SLC_COMMA, __VA_ARGS__ )                                                                     \
			};                                                                                                                                    \
                                                                                                                                                  \
                                                                                                                                                  \
		public:                                                                                                                                   \
			SLC_FOR_EACH( SLC_MAKE_INTERNAL_ENUM, SLC_FOR_EACH_SEP( SLC_EXTRACT_IDENT, SLC_COMMA, __VA_ARGS__ ) )                                 \
                                                                                                                                                  \
		private:                                                                                                                                  \
			using ValueTypes = ::slc::TypeList< Ts... >;                                                                                          \
			using Self = name##_RustEnum< ValueTypes >;                                                                                           \
			using ValueStorageType = ValueTypes::VariantType;                                                                                     \
                                                                                                                                                  \
			template < Enum Element >                                                                                                             \
			using ValueTypeAt = typename ValueTypes::template Type< ::slc::detail::EnumTag< Element >::Value >;                                   \
                                                                                                                                                  \
			template < Enum Element >                                                                                                             \
			SCONSTEXPR bool HasType = !( std::same_as< ValueTypeAt< Element >, std::monostate > );                                                \
                                                                                                                                                  \
			template < typename R >                                                                                                               \
			SCONSTEXPR bool ContainsType = ValueTypes::template Contains< R >;                                                                    \
                                                                                                                                                  \
			template < typename F, Enum Element >                                                                                                 \
			SCONSTEVAL bool ComputeHasMatchFunc()                                                                                                 \
			{                                                                                                                                     \
				if constexpr ( HasType< Element > )                                                                                               \
					return std::invocable< F, ::slc::detail::EnumTag< Element >, const ValueTypeAt< Element >& >;                                 \
				else                                                                                                                              \
					return std::invocable< F, ::slc::detail::EnumTag< Element > >;                                                                \
			}                                                                                                                                     \
                                                                                                                                                  \
			template < typename F, Enum Element >                                                                                                 \
			static consteval bool ComputeHasDefaultMatchFuncWithArg()                                                                             \
			{                                                                                                                                     \
				using DefaultHandler = typename ::slc::detail::ExtractDefaultHandler< F >::type;                                                  \
				if constexpr ( !std::is_void_v< DefaultHandler > )                                                                                \
				{                                                                                                                                 \
					using Callable = decltype( std::declval< DefaultHandler >().func );                                                           \
					return std::invocable< Callable, const ValueTypeAt< Element >& >;                                                             \
				}                                                                                                                                 \
				else                                                                                                                              \
				{                                                                                                                                 \
					return false;                                                                                                                 \
				}                                                                                                                                 \
			}                                                                                                                                     \
			template < typename F, Enum Element >                                                                                                 \
			static consteval bool ComputeHasDefaultMatchFuncNoArg()                                                                               \
			{                                                                                                                                     \
				using DefaultHandler = typename ::slc::detail::ExtractDefaultHandler< F >::type;                                                  \
				if constexpr ( !std::is_void_v< DefaultHandler > )                                                                                \
				{                                                                                                                                 \
					using Callable = decltype( std::declval< DefaultHandler >().func );                                                           \
					return std::invocable< Callable >;                                                                                            \
				}                                                                                                                                 \
				else                                                                                                                              \
				{                                                                                                                                 \
					return false;                                                                                                                 \
				}                                                                                                                                 \
			}                                                                                                                                     \
                                                                                                                                                  \
		public:                                                                                                                                   \
			template < typename F, Enum Element >                                                                                                 \
			SCONSTEXPR bool HasMatchFunc = ComputeHasMatchFunc< F, Element >();                                                                   \
                                                                                                                                                  \
			template < typename F, Enum Element >                                                                                                 \
			SCONSTEXPR bool HasDefaultMatchFuncWithArg = ComputeHasDefaultMatchFuncWithArg< F, Element >();                                       \
			template < typename F, Enum Element >                                                                                                 \
			SCONSTEXPR bool HasDefaultMatchFuncNoArg = ComputeHasDefaultMatchFuncNoArg< F, Element >();                                           \
                                                                                                                                                  \
		public:                                                                                                                                   \
			constexpr name##_RustEnum() = default;                                                                                                \
			constexpr name##_RustEnum( name##_RustEnum const& ) = default;                                                                        \
			constexpr name##_RustEnum( name##_RustEnum&& ) = default;                                                                             \
			name##_RustEnum& operator=( name##_RustEnum const& other )                                                                            \
			{                                                                                                                                     \
				mValueData = other.mValueData;                                                                                                    \
				return *this;                                                                                                                     \
			}                                                                                                                                     \
			name##_RustEnum& operator=( name##_RustEnum&& other ) noexcept                                                                        \
			{                                                                                                                                     \
				mValueData = std::move( other.mValueData );                                                                                       \
				return *this;                                                                                                                     \
			}                                                                                                                                     \
			~name##_RustEnum() = default;                                                                                                         \
                                                                                                                                                  \
			template < Enum Element, typename... Args >                                                                                           \
			constexpr name##_RustEnum( ::slc::detail::EnumTag< Element >, Args&&... args )                                                        \
				: mValueData( std::in_place_index_t< ::slc::detail::EnumTag< Element >::Value >{}, std::forward< Args >( args )... )              \
			{                                                                                                                                     \
			}                                                                                                                                     \
                                                                                                                                                  \
			template < typename... Cases >                                                                                                        \
			void Match( Cases&&... cases ) const                                                                                                  \
			{                                                                                                                                     \
				auto matcher = ::slc::detail::Overload{ std::forward< Cases >( cases )... };                                                      \
				using Matcher = decltype( matcher );                                                                                              \
                                                                                                                                                  \
				switch ( mValueData.index() )                                                                                                     \
				{                                                                                                                                 \
					SLC_FOR_EACH( SLC_MATCH_CASE, SLC_FOR_EACH_SEP( SLC_EXTRACT_IDENT, SLC_COMMA, __VA_ARGS__ ) )                                 \
				}                                                                                                                                 \
			}                                                                                                                                     \
                                                                                                                                                  \
			template < Enum Element >                                                                                                             \
			ValueTypeAt< Element >& Unwrap( ::slc::detail::EnumTag< Element > )                                                                   \
			{                                                                                                                                     \
				return *std::get_if< ::slc::detail::EnumTag< Element >::Value >( &mValueData );                                                   \
			}                                                                                                                                     \
                                                                                                                                                  \
			template < Enum Element >                                                                                                             \
			ValueTypeAt< Element > const& Unwrap( ::slc::detail::EnumTag< Element > ) const                                                       \
			{                                                                                                                                     \
				return *std::get_if< ::slc::detail::EnumTag< Element >::Value >( &mValueData );                                                   \
			}                                                                                                                                     \
                                                                                                                                                  \
		private:                                                                                                                                  \
			template < Enum Element, typename Matcher >                                                                                           \
			void DispatchCase( Matcher&& matcher ) const                                                                                          \
			{                                                                                                                                     \
				if constexpr ( HasMatchFunc< Matcher, Element > )                                                                                 \
				{                                                                                                                                 \
					if constexpr ( HasType< Element > )                                                                                           \
					{                                                                                                                             \
						std::forward< Matcher >( matcher )( ::slc::detail::EnumTag< Element >{}, Unwrap( ::slc::detail::EnumTag< Element >{} ) ); \
					}                                                                                                                             \
					else                                                                                                                          \
					{                                                                                                                             \
						std::forward< Matcher >( matcher )( ::slc::detail::EnumTag< Element >{} );                                                \
					}                                                                                                                             \
				}                                                                                                                                 \
				else if constexpr ( HasDefaultMatchFuncWithArg< Matcher, Element > )                                                              \
				{                                                                                                                                 \
					std::forward< Matcher >( matcher )( std::monostate{}, Unwrap( ::slc::detail::EnumTag< Element >{} ) );                        \
				}                                                                                                                                 \
				else if constexpr ( HasDefaultMatchFuncNoArg< Matcher, Element > )                                                                \
				{                                                                                                                                 \
					std::forward< Matcher >( matcher )( std::monostate{} );                                                                       \
				}                                                                                                                                 \
				else                                                                                                                              \
				{                                                                                                                                 \
					static_assert( AlwaysFalse< Matcher >, "Must provide a match case for all cases" );                                           \
				}                                                                                                                                 \
			}                                                                                                                                     \
                                                                                                                                                  \
		private:                                                                                                                                  \
			ValueStorageType mValueData;                                                                                                          \
		};                                                                                                                                        \
                                                                                                                                                  \
		using Impl = name##_RustEnum< ::slc::TypeList< SLC_FOR_EACH_SEP( SLC_EXTRACT_TYPE, SLC_COMMA, __VA_ARGS__ ) > >;                          \
	};                                                                                                                                            \
	using name = detail_##name::Impl;

/*
	SLC_MAKE_SMART_ENUM(Error,
		OutOfBounds,
		Unexpected(std::string)
	)

	Error test = Error::OutOfBounds;
*/
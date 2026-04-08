#pragma once

#include <functional>
#include <variant>
#include <string_view>
#include <source_location>

#define SL_FUNC_SIG_STRING                              \
	std::string_view                                    \
	{                                                   \
		std::source_location::current().function_name() \
	}

namespace sl {

	namespace detail {

		static consteval std::string_view Extract( std::string_view sv, std::string_view prefix, std::string_view suffix )
		{
			auto start = sv.find( prefix );

			if ( start == std::string_view::npos )
				start = 0;
			else
				start += prefix.size();

			auto end = sv.rfind( suffix );
			if ( suffix.empty() || end == std::string_view::npos || end <= start )
				end = sv.size();

			return sv.substr( start, end - start );
		}

		static consteval std::string_view Trim( std::string_view sv, std::string_view prefix, std::string_view suffix )
		{
			std::size_t start = sv.starts_with( prefix ) ? prefix.size() : 0;
			std::size_t end = sv.ends_with( suffix ) ? sv.rfind( suffix ) : sv.size();

			return sv.substr( start, end - start );
		}

		template < typename Type >
		static consteval auto GetName() noexcept
		{
#if defined( __clang__ ) || defined( __GNUC__ )
			// Example GCC/Clang __PRETTY_FUNCTION__:
			// "consteval std::string_view detail::GetLongName() [with T = Foo]"
			constexpr std::string_view prefix = "T = ";
			constexpr std::string_view suffix = "]";
			return Extract( SL_FUNC_SIG_STRING, prefix, suffix );

#elif defined( _MSC_VER )
			// Example MSVC __FUNCSIG__:
			// "consteval std::string_view __cdecl detail::GetLongName<struct Foo>(void)"
			constexpr std::string_view prefix1 = "GetName<";
			constexpr std::string_view prefix2 = "class ";
			constexpr std::string_view prefix3 = "struct ";
			constexpr std::string_view suffix = ">(void)";
			return Trim( Trim( Extract( SL_FUNC_SIG_STRING, prefix1, suffix ), prefix2, {} ), prefix3, {} );
#endif
		}

		template < typename T >
		inline constexpr std::monostate TypeTagData;

		template < typename T >
		inline constexpr void const* TypeTagDataPtr = &TypeTagData< T >;
	} // namespace detail

	/*
		Type Traits for inspecting a given type
	*/

	using TypeTag = void const*;

	template < typename T >
	struct TypeTraits
	{
		using Type = T;
		using BaseType = std::remove_cvref_t< T >;

		static constexpr TypeTag Tag = detail::TypeTagDataPtr< Type >;
		static constexpr TypeTag BaseTag = detail::TypeTagDataPtr< BaseType >;

		static constexpr auto Name = detail::GetName< Type >();
		static constexpr auto BaseName = detail::GetName< std::remove_cvref_t< Type > >();

		static constexpr bool IsObject = std::is_class_v< Type >;
		static constexpr bool IsReference = std::is_reference_v< Type >;
		static constexpr bool IsLValueReference = std::is_lvalue_reference_v< Type >;
		static constexpr bool IsRValueReference = std::is_rvalue_reference_v< Type >;
		static constexpr bool IsPointer = std::is_pointer_v< Type >;
		static constexpr bool IsEnum = std::is_enum_v< Type >;
		static constexpr bool IsArray = std::is_array_v< Type >;
		static constexpr bool IsConst = std::is_const_v< std::remove_reference_t< Type > >;
		static constexpr bool IsStandard = std::is_standard_layout_v< Type >;

		template < typename R >
		static constexpr bool IsBaseOf = std::is_base_of_v< Type, R >;

		template < typename R >
		static constexpr bool IsSameAs = std::is_same_v< Type, R >;
	};

	namespace detail {

		template < size_t I, typename T, typename TupleType >
		static consteval size_t IndexFunction()
		{
			static_assert( I < std::tuple_size_v< TupleType >, "The element is not in the tuple" );

			using IndexType = typename std::tuple_element< I, TupleType >::type;

			if constexpr ( std::is_same_v< T, IndexType > )
				return I;
			else
				return IndexFunction< I + 1, T, TupleType >();
		}
	} // namespace detail


	/*
		A type list for inspecting a list of types.
	*/

	/*
			A type list for inspecting a list of types.
			Nested TypeList arguments are flattened automatically.
		*/

	template < typename... Ts >
	struct TypeList;

	template < typename... Ts >
	struct RawTypeList;


	enum class SetComparison
	{
		Subset,
		Superset,
		Equal
	};

	namespace detail {

		template < typename T >
		struct IsTypeListImpl : std::false_type
		{};

		template < typename... Ts >
		struct IsTypeListImpl< TypeList< Ts... > > : std::true_type
		{};

		template < typename... Ts >
		struct IsTypeListImpl< RawTypeList< Ts... > > : std::true_type
		{};

		template < typename T >
		inline constexpr bool IsTypeListV = IsTypeListImpl< std::remove_cvref_t< T > >::value;


		template < SetComparison TOp, typename TBase, typename TOther >
		static consteval bool CompareByImpl()
		{
			using Base = std::remove_cvref_t< TBase >;
			using Other = std::remove_cvref_t< TOther >;

			if constexpr ( TOp == SetComparison::Subset )
			{
				return Base::template All(
					[]< typename T >() constexpr {
						return Base::template Contains< std::remove_cvref_t< T > >;
					}
				);
			}
			else if constexpr ( TOp == SetComparison::Superset )
			{
				return Other::All(
					[]< typename T >() constexpr {
						return Base::template Contains< std::remove_cvref_t< T > >;
					}
				);
			}
			else if constexpr ( TOp == SetComparison::Equal )
			{
				return CompareByImpl< SetComparison::Subset, Base, Other >() &&
					   CompareByImpl< SetComparison::Superset, Base, Other >();
			}
			else
			{
				return false;
			}
		}

		template < typename... Ts >
		struct TypeListStorage
		{
			static constexpr size_t Size = sizeof...( Ts );

			using TupleType = std::tuple< Ts... >;
			using VariantType = std::variant< Ts... >;
			using VariantTypeWithMonostate = std::variant< std::monostate, Ts... >;

			template < typename R >
			static constexpr bool Contains = std::disjunction_v< std::is_same< R, Ts >... >;

			template < typename R >
			static constexpr size_t Index = IndexFunction< 0, R, TupleType >();

			template < size_t I >
			using TypeAt = typename std::tuple_element< I, TupleType >::type;

			template < size_t I >
			using TraitsAt = TypeTraits< typename std::tuple_element< I, TupleType >::type >;


			template < SetComparison TOp, typename TOther >
				requires IsTypeListV< TOther >
			static constexpr bool CompareBy = CompareByImpl< TOp, TypeListStorage< Ts... >, TOther >();


			template < typename Func >
				requires( requires( Func&& func ) { { std::forward< Func >( func ).template operator()< Ts >() } -> std::same_as< void >; } && ... )
			static constexpr void ForEach( Func&& func )
			{
				( std::forward< Func >( func ).template operator()< Ts >(), ... );
			}

			template < typename Pred >
				requires( requires( Pred&& pred ) { { std::forward< Pred >( pred ).template operator()< Ts >() } -> std::convertible_to< bool >; } && ... )
			static constexpr bool Any( Pred&& pred )
			{
				return ( ... || static_cast< bool >( std::forward< Pred >( pred ).template operator()< Ts >() ) );
			}

			template < typename Pred >
				requires( requires( Pred&& pred ) { { std::forward< Pred >( pred ).template operator()< Ts >() } -> std::convertible_to< bool >; } && ... )
			static constexpr bool All( Pred&& pred )
			{
				return ( ... && static_cast< bool >( std::forward< Pred >( pred ).template operator()< Ts >() ) );
			}
		};

		template < typename... Ts >
		struct TypeListCat;

		template <>
		struct TypeListCat<>
		{
			using Type = TypeListStorage<>;
		};

		template < typename T, typename... Rest >
		struct TypeListCat< T, Rest... >
		{
		private:
			using Head = TypeListStorage< T >;
			using Tail = typename TypeListCat< Rest... >::Type;

			template < typename H, typename TTail >
			struct Merge;

			template < typename... Hs, typename... Ts >
			struct Merge< TypeListStorage< Hs... >, TypeListStorage< Ts... > >
			{
				using Type = TypeListStorage< Hs..., Ts... >;
			};

		public:
			using Type = typename Merge< Head, Tail >::Type;
		};

		template < typename... Inner, typename... Rest >
		struct TypeListCat< TypeList< Inner... >, Rest... >
		{
		private:
			using Head = typename TypeListCat< Inner... >::Type;
			using Tail = typename TypeListCat< Rest... >::Type;

			template < typename H, typename TTail >
			struct Merge;

			template < typename... Hs, typename... Ts >
			struct Merge< TypeListStorage< Hs... >, TypeListStorage< Ts... > >
			{
				using Type = TypeListStorage< Hs..., Ts... >;
			};

		public:
			using Type = typename Merge< Head, Tail >::Type;
		};


		template < bool Flatten, typename... Ts >
		using TypeListImpl = std::conditional_t< Flatten, typename TypeListCat< Ts... >::Type, TypeListStorage< Ts... > >;

	} // namespace detail

	/*
	 *	A type list for inspecting a list of types. Can pass nested TypeLists and they will be flattened automatically.
	 */

	template < typename... Ts >
	struct TypeList : detail::TypeListImpl< /*Flatten=*/true, Ts... >
	{};

	template < typename... Ts >
	struct RawTypeList : detail::TypeListImpl< /*Flatten=*/false, Ts... >
	{};

	template < typename T >
	concept IsTypeList = detail::IsTypeListV< T >;

	/*
		General purpose conceptzs
	*/

	template < typename T >
	concept IsEnum = TypeTraits< T >::IsEnum;

	template < typename T, typename Base >
	concept DerivedFromOnly = std::derived_from< T, Base > and not std::same_as< T, Base >;

	template < typename T >
	concept IsStandard = std::is_standard_layout_v< T >;

	template < typename T >
	concept IsConst = std::is_const_v< T >;

	template < typename From, typename To >
	concept Castable = requires( From from ) { static_cast< To >( from ); };

	template < typename T >
	concept Integral = std::integral< T >;

	template < typename T >
	concept Numeric = std::is_arithmetic_v< T >;

	template < typename T >
	concept AddAssignable = requires( T&& t ) {
		t += t;
	};

	template < typename T >
	concept UnaryAddable = requires( T&& t ) {
		{ t + t } -> std::convertible_to< T >;
	};

	template < typename T >
	concept Summable = std::is_default_constructible_v< T > and ( AddAssignable< T > or UnaryAddable< T > );

	template < typename T >
	concept UNumeric = std::unsigned_integral< T >;

	template < typename T >
	concept ComparableLess = requires( T&& t1, T&& t2 ) { std::less< T >( t1, t2 ); };

	template < typename T >
	concept ComparableGreater = requires( T&& t1, T&& t2 ) { std::greater< T >( t1, t2 ); };

	template < typename T >
	concept Sizeable = requires( T&& t ) {
		{ std::size( t ) } -> std::convertible_to< size_t >;
	};


	template < typename Func, typename TReturn, typename... TArgs >
	concept IsFunc = std::invocable< Func, TArgs... > and std::convertible_to< std::invoke_result_t< Func, TArgs... >, TReturn >;

	template < typename Func, typename... TArgs >
	concept IsAction = IsFunc< Func, void, TArgs... >;

	template < typename Func, typename... TArgs >
	concept IsPredicate = IsFunc< Func, bool, TArgs... >;


	template < typename H, typename Key >
	concept Hash =
		std::regular_invocable< const H&, const Key& > &&
		std::convertible_to< std::invoke_result_t< const H&, const Key& >, std::size_t >;

	namespace detail {
		template < typename F >
		struct OverloadLeaf
		{
			using Stored = std::decay_t< F >;

			Stored func;

			template < typename Fn >
			constexpr explicit OverloadLeaf( Fn&& f )
				: func( std::forward< Fn >( f ) )
			{
			}

			template < typename... Args >
				requires std::invocable< Stored&, Args... >
			constexpr decltype( auto ) operator()( Args&&... args ) & noexcept( noexcept( std::invoke( func, std::forward< Args >( args )... ) ) )
			{
				return std::invoke( func, std::forward< Args >( args )... );
			}

			template < typename... Args >
				requires std::invocable< Stored const&, Args... >
			constexpr decltype( auto ) operator()( Args&&... args ) const& noexcept( noexcept( std::invoke( func, std::forward< Args >( args )... ) ) )
			{
				return std::invoke( func, std::forward< Args >( args )... );
			}

			template < typename... Args >
				requires std::invocable< Stored, Args... >
			constexpr decltype( auto ) operator()( Args&&... args ) && noexcept( noexcept( std::invoke( std::move( func ), std::forward< Args >( args )... ) ) )
			{
				return std::invoke( std::move( func ), std::forward< Args >( args )... );
			}

			template < typename... Args >
				requires std::invocable< Stored const, Args... >
			constexpr decltype( auto ) operator()( Args&&... args ) const&& noexcept( noexcept( std::invoke( std::move( func ), std::forward< Args >( args )... ) ) )
			{
				return std::invoke( std::move( func ), std::forward< Args >( args )... );
			}
		};

	} // namespace detail

	template < typename... Fs >
	struct Overload : detail::OverloadLeaf< Fs >...
	{
		using detail::OverloadLeaf< Fs >::operator()...;

		template < typename... Gs >
		constexpr explicit Overload( Gs&&... fs )
			: detail::OverloadLeaf< Fs >( std::forward< Gs >( fs ) )...
		{
		}
	};

	template < typename... Fs >
	Overload( Fs&&... ) -> Overload< std::decay_t< Fs >... >;


	/*
		Function Traits for inspecting a function type
	*/

	template < typename T, typename = void >
	struct FunctionTraits
	{
		static constexpr bool Valid = false;
	};

	// Functor / lambda fallback.
	// Works for non-generic, non-overloaded operator().
	template < typename T >
	struct FunctionTraits< T, std::void_t< decltype( &std::remove_cvref_t< T >::operator() ) > >
		: FunctionTraits< decltype( &std::remove_cvref_t< T >::operator() ) >
	{
	};

	// Plain function type
	template < typename R, typename... Args >
	struct FunctionTraits< R( Args... ), void >
	{
		using ReturnType = R;
		using Arguments = RawTypeList< Args... >;
		using FunctionType = R( Args... );

		static constexpr bool Valid = true;
		static constexpr bool IsNoexcept = false;
		static constexpr bool IsMemberFunction = false;
	};

	// Plain function type noexcept
	template < typename R, typename... Args >
	struct FunctionTraits< R( Args... ) noexcept, void >
	{
		using ReturnType = R;
		using Arguments = RawTypeList< Args... >;

		static constexpr bool IsNoexcept = true;
		static constexpr bool IsMemberFunction = false;
	};

	// Function pointer
	template < typename R, typename... Args >
	struct FunctionTraits< R ( * )( Args... ), void > : FunctionTraits< R( Args... ) >
	{
	};

	// Function pointer noexcept
	template < typename R, typename... Args >
	struct FunctionTraits< R ( * )( Args... ) noexcept, void > : FunctionTraits< R( Args... ) noexcept >
	{
	};

	// Function lvalue reference
	template < typename R, typename... Args >
	struct FunctionTraits< R ( & )( Args... ), void > : FunctionTraits< R( Args... ) >
	{
	};

	// Function lvalue reference noexcept
	template < typename R, typename... Args >
	struct FunctionTraits< R ( & )( Args... ) noexcept, void > : FunctionTraits< R( Args... ) noexcept >
	{
	};

	// Function rvalue reference
	template < typename R, typename... Args >
	struct FunctionTraits< R ( && )( Args... ), void > : FunctionTraits< R( Args... ) >
	{
	};

	// Function rvalue reference noexcept
	template < typename R, typename... Args >
	struct FunctionTraits< R ( && )( Args... ) noexcept, void > : FunctionTraits< R( Args... ) noexcept >
	{
	};

	// std::function
	template < typename R, typename... Args >
	struct FunctionTraits< std::function< R( Args... ) >, void > : FunctionTraits< R( Args... ) >
	{
	};

#define SL_FUNCTION_TRAITS_MEMBER( CV_QUAL, REF_QUAL, NOEXCEPT_SPEC, IS_NOEXCEPT_VALUE )    \
	template < typename R, typename O, typename... Args >                                   \
	struct FunctionTraits< R ( O::* )( Args... ) CV_QUAL REF_QUAL NOEXCEPT_SPEC, void >     \
	{                                                                                       \
		using ObjectType = CV_QUAL O;                                                       \
		using ClassType = O;                                                                \
		using ReturnType = R;                                                               \
		using Arguments = RawTypeList< Args... >;                                           \
		using MemberFunctionPointer = R ( O::* )( Args... ) CV_QUAL REF_QUAL NOEXCEPT_SPEC; \
                                                                                            \
                                                                                            \
		static constexpr bool Valid = true;                                                 \
		static constexpr bool IsNoexcept = IS_NOEXCEPT_VALUE;                               \
		static constexpr bool IsMemberFunction = true;                                      \
		static constexpr bool IsConst = std::is_const_v< CV_QUAL O >;                       \
		static constexpr bool IsVolatile = std::is_volatile_v< CV_QUAL O >;                 \
		static constexpr bool IsLvalueQualified = std::is_same_v< int REF_QUAL, int& >;     \
		static constexpr bool IsRvalueQualified = std::is_same_v< int REF_QUAL, int&& >;    \
	};

	// no cv, no ref
	SL_FUNCTION_TRAITS_MEMBER(, , , false )
	SL_FUNCTION_TRAITS_MEMBER(, , noexcept, true )

	// no cv, &
	SL_FUNCTION_TRAITS_MEMBER(, &, , false )
	SL_FUNCTION_TRAITS_MEMBER(, &, noexcept, true )

	// no cv, &&
	SL_FUNCTION_TRAITS_MEMBER(, &&, , false )
	SL_FUNCTION_TRAITS_MEMBER(, &&, noexcept, true )

	// const
	SL_FUNCTION_TRAITS_MEMBER( const, , , false )
	SL_FUNCTION_TRAITS_MEMBER( const, , noexcept, true )

	// const &
	SL_FUNCTION_TRAITS_MEMBER( const, &, , false )
	SL_FUNCTION_TRAITS_MEMBER( const, &, noexcept, true )

	// const &&
	SL_FUNCTION_TRAITS_MEMBER( const, &&, , false )
	SL_FUNCTION_TRAITS_MEMBER( const, &&, noexcept, true )

	// volatile
	SL_FUNCTION_TRAITS_MEMBER( volatile, , , false )
	SL_FUNCTION_TRAITS_MEMBER( volatile, , noexcept, true )

	// volatile &
	SL_FUNCTION_TRAITS_MEMBER( volatile, &, , false )
	SL_FUNCTION_TRAITS_MEMBER( volatile, &, noexcept, true )

	// volatile &&
	SL_FUNCTION_TRAITS_MEMBER( volatile, &&, , false )
	SL_FUNCTION_TRAITS_MEMBER( volatile, &&, noexcept, true )

	// const volatile
	SL_FUNCTION_TRAITS_MEMBER( const volatile, , , false )
	SL_FUNCTION_TRAITS_MEMBER( const volatile, , noexcept, true )

	// const volatile &
	SL_FUNCTION_TRAITS_MEMBER( const volatile, &, , false )
	SL_FUNCTION_TRAITS_MEMBER( const volatile, &, noexcept, true )

	// const volatile &&
	SL_FUNCTION_TRAITS_MEMBER( const volatile, &&, , false )
	SL_FUNCTION_TRAITS_MEMBER( const volatile, &&, noexcept, true )

#undef SL_FUNCTION_TRAITS_MEMBER


	/*
		Property Traits for inspecting a data member
	*/

	template < typename T >
	struct PropertyTraits;

	// Member object pointer specialisation
	template < typename P, typename O >
	struct PropertyTraits< P O::* >
	{
		using ObjectType = O;
		using PropType = P;
	};
} // namespace sl
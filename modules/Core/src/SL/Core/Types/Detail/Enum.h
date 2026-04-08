#pragma once

#include <SL/Core/Common/Base.h>

namespace sl {
	template < IsEnum Enum, typename... Specs >
	class SmartEnum;
}

namespace sl::detail {

	template < auto E >
		requires std::is_enum_v< decltype( E ) >
	struct EnumTag : std::integral_constant< decltype( E ), E >
	{
		static consteval std::size_t ComputeIndex()
		{
			constexpr auto maybe_index = magic_enum::enum_index( E );
			static_assert( maybe_index.has_value(), "EnumTag: value not reflectable by magic_enum" );
			return static_cast< std::size_t >( *maybe_index );
		}

		static constexpr std::size_t Index = ComputeIndex();
	};

	template < auto E, typename F >
	struct EnumMatchCaseHandler
	{
		F func;
		static constexpr auto Element = E;

		template < typename... Args >
			requires std::invocable< F const&, Args... >
		decltype( auto ) operator()( EnumTag< E >, Args&&... args ) const
		{
			return func( std::forward< Args >( args )... );
		}
	};

	template < typename F >
	struct EnumDefaultMatchCaseHandler
	{
		F func;

		decltype( auto ) operator()( std::monostate ) const
			requires std::invocable< F const& >
		{
			return func();
		}
	};

	template < typename T >
	struct IsEnumMatchCaseHandler : std::false_type
	{};

	template < auto E, typename F >
	struct IsEnumMatchCaseHandler< EnumMatchCaseHandler< E, F > > : std::true_type
	{};

	template < typename T >
	struct IsOverload : std::false_type
	{};

	template < typename... Fs >
	struct IsOverload< Overload< Fs... > > : std::true_type
	{};

	template < typename T >
	struct IsDefaultMatchCaseHandler : std::false_type
	{};

	template < typename F >
	struct IsDefaultMatchCaseHandler< EnumDefaultMatchCaseHandler< F > > : std::true_type
	{};

	template < typename... Ts >
	struct FindDefaultHandlerHelper
	{
		using type = void;
	};

	template < typename T, typename... Rest >
	struct FindDefaultHandlerHelper< T, Rest... >
	{
	private:
		using CleanT = std::remove_cvref_t< T >;

	public:
		using type = std::conditional_t<
			IsDefaultMatchCaseHandler< CleanT >::value,
			CleanT,
			typename FindDefaultHandlerHelper< Rest... >::type >;
	};

	template < typename T >
	struct ExtractDefaultHandler;

	template < template < typename... > class OverloadT, typename... Cases >
	struct ExtractDefaultHandler< OverloadT< Cases... > >
	{
		using type = typename FindDefaultHandlerHelper< Cases... >::type;
	};

	template < typename T, typename Enum >
	struct IsRelevantEnumMatchCaseHandler : std::false_type
	{};

	template < auto E, typename F, typename Enum >
	struct IsRelevantEnumMatchCaseHandler< EnumMatchCaseHandler< E, F >, Enum >
		: std::bool_constant< std::same_as< std::remove_cv_t< decltype( E ) >, Enum > >
	{};

	template < typename T, typename Enum >
	inline constexpr bool IsRelevantEnumMatchCaseHandlerV =
		IsRelevantEnumMatchCaseHandler< std::remove_cvref_t< T >, Enum >::value;

	template < auto EnumValue, typename Underlying = std::monostate >
		requires std::is_enum_v< decltype( EnumValue ) >
	struct Variant
	{
		static constexpr auto Tag = EnumTag< EnumValue >{};
		static constexpr auto Value = EnumValue;
		using Type = Underlying;
	};

	template < typename T >
	struct IsVariant : std::false_type
	{};

	template < auto E, typename U >
	struct IsVariant< Variant< E, U > > : std::true_type
	{};

	template < typename Derived, IsEnum Enum, typename... Variants >
	class SmartEnumImpl
	{
	private:
		static consteval bool AllVariantsUniqueAndValid()
		{
			constexpr bool AllValidVariants = ( ... && IsVariant< Variants >::value );
			static_assert( AllValidVariants, "All types must be sl::Case<E> or sl::Case<E, T>" );

			static constexpr bool AllCorrectEnumTypes =
				( ... && std::same_as< std::remove_cv_t< decltype( Variants::Value ) >, Enum > );
			static_assert( AllCorrectEnumTypes, "SmartEnum spec enum value must belong to the SmartEnum enum type." );

			constexpr auto CaseValues = std::array{ Variants::Value... };
			constexpr auto EnumValues = magic_enum::enum_values< Enum >();

			constexpr auto CaseCounts = [ = ] {
				std::array< std::size_t, magic_enum::enum_count< Enum >() > counts{};
				for ( std::size_t i = 0; i < EnumValues.size(); ++i )
					counts[ i ] = std::ranges::count( CaseValues, EnumValues[ i ] );
				return counts;
			}();

			constexpr auto ZeroCount = []( std::size_t count ) { return count == 0; };
			constexpr auto MultipleCount = []( std::size_t count ) { return count > 1; };

			static constexpr bool HasAllEnumValues = !std::ranges::any_of( CaseCounts, ZeroCount );
			static_assert( HasAllEnumValues, "SmartEnum must provide a case for every enum element" );

			static constexpr bool HasNoDuplicates = !std::ranges::any_of( CaseCounts, MultipleCount );
			static_assert( HasNoDuplicates, "SmartEnum must provide only one case for each enum element" );

			return true;
		}

		static_assert( AllVariantsUniqueAndValid(), "SmartEnum cases do not satisfy conditions" );

		template < Enum V, typename... Ts >
		struct FindVariant;

		template < Enum V, Enum E, typename U, typename... Rest >
		struct FindVariant< V, Variant< E, U >, Rest... >
			: std::conditional_t< ( E == V ), std::type_identity< Variant< E, U > >, FindVariant< V, Rest... > >
		{};

		template < Enum V >
		struct FindVariant< V >
		{};

		static constexpr auto EnumValues = magic_enum::enum_values< Enum >();

		template < std::size_t I >
		using OrderedCaseAt = typename FindVariant< EnumValues[ I ], Variants... >::type;

		template < std::size_t... Is >
		static consteval auto MakeOrderedCaseTypes( std::index_sequence< Is... > )
		{
			return std::type_identity< TypeList< OrderedCaseAt< Is >... > >{};
		}

		using CaseTypes = typename decltype( MakeOrderedCaseTypes( std::make_index_sequence< magic_enum::enum_count< Enum >() >{} ) )::type;

		template < std::size_t I >
		using OrderedValueAt = typename OrderedCaseAt< I >::Type;

		template < std::size_t... Is >
		static consteval auto MakeOrderedValueTypes( std::index_sequence< Is... > )
		{
			return std::type_identity< TypeList< OrderedValueAt< Is >... > >{};
		}

		using ValueTypes = typename decltype( MakeOrderedValueTypes( std::make_index_sequence< magic_enum::enum_count< Enum >() >{} ) )::type;

	public:
		using StorageType = typename ValueTypes::VariantType;

	private:
		template < std::size_t I >
		using CaseTypeAt = typename CaseTypes::template TypeAt< I >;

		template < Enum Element >
		static constexpr std::size_t CaseIndex = EnumTag< Element >::Index;

		template < Enum Element >
		using ValueTypeAt = typename ValueTypes::template TypeAt< CaseIndex< Element > >;

		template < Enum Element >
		static constexpr bool HasType = !( std::same_as< ValueTypeAt< Element >, std::monostate > );

		template < typename... Ts >
		struct HasCommonTypeDetail
		{
			template < typename... Us >
			static auto Test( int ) -> decltype( void( typename std::common_type< Us... >::type{} ), std::true_type{} );

			template < typename... >
			static auto Test( ... ) -> std::false_type;

			static constexpr bool Value = decltype( Test< Ts... >( 0 ) )::value;
		};

		template < typename... Ts >
		static constexpr bool HasCommonType = HasCommonTypeDetail< Ts... >::Value;

		template < typename F, Enum Element >
		static consteval bool ComputeHasMatchFunc()
		{
			if constexpr ( HasType< Element > )
				return std::invocable< F, EnumTag< Element >, const ValueTypeAt< Element >& >;
			else
				return std::invocable< F, EnumTag< Element > >;
		}

		template < typename F >
		static consteval bool ComputeHasDefaultMatchFuncNoArg()
		{
			using DefaultHandler = typename ExtractDefaultHandler< F >::type;
			if constexpr ( !std::is_void_v< DefaultHandler > )
			{
				using Callable = decltype( std::declval< DefaultHandler >().func );
				return std::invocable< Callable >;
			}
			else
			{
				return false;
			}
		}

		template < typename F, Enum Element >
		static constexpr bool HasMatchFunc = ComputeHasMatchFunc< F, Element >();

		template < typename F >
		static constexpr bool HasDefaultMatchFuncNoArg = ComputeHasDefaultMatchFuncNoArg< F >();

		template < std::size_t I, typename Matcher >
		static consteval decltype( auto ) SimulateDispatchEntry()
		{
			using Case = CaseTypeAt< I >;
			using T = typename Case::Type;

			return +[]( const Matcher& matcher, const StorageType& variant ) -> decltype( auto ) {
				constexpr auto tag = Case::Tag;
				constexpr auto enum_val = decltype( tag )::value;

				if constexpr ( HasMatchFunc< Matcher, enum_val > )
				{
					if constexpr ( !std::same_as< T, std::monostate > )
						return matcher( tag, *std::get_if< I >( &variant ) );
					else
						return matcher( tag );
				}
				else if constexpr ( HasDefaultMatchFuncNoArg< Matcher > )
				{
					return matcher( std::monostate{} );
				}
				else
				{
					std::unreachable();
				}
			};
		}

		template < typename Matcher, std::size_t... Is >
		static consteval auto ComputeDispatchReturnType( std::index_sequence< Is... > )
		{
			using ResultType = std::common_type_t<
				decltype( SimulateDispatchEntry< Is, Matcher >()( std::declval< Matcher >(), std::declval< StorageType >() ) )... >;

			return std::type_identity< ResultType >{};
		}

		template < typename Matcher, std::size_t... Is >
		static consteval auto TryComputeDispatchReturnType( std::index_sequence< Is... > )
		{
			constexpr bool HandlersHaveCommonType =
				HasCommonType< decltype( SimulateDispatchEntry< Is, Matcher >()( std::declval< Matcher >(), std::declval< StorageType >() ) )... >;

			SL_COMPILE_CHECK(
				HandlersHaveCommonType,
				SmartEnum::Match,
				"Match case handlers must return the same (or implicitly convertible) type for all variants."
			);

			if constexpr ( !HandlersHaveCommonType )
				return std::type_identity< void >{};
			else
				return ComputeDispatchReturnType< Matcher >( std::index_sequence< Is... >{} );
		}

		template < std::size_t I, typename Matcher, typename ReturnType >
		static constexpr decltype( auto ) MakeDispatchEntry()
		{
			using Case = CaseTypeAt< I >;
			using T = typename Case::Type;

			return +[]( const Matcher& matcher, const StorageType& variant ) -> ReturnType {
				constexpr auto tag = Case::Tag;
				constexpr auto enum_val = decltype( tag )::value;

				if constexpr ( HasMatchFunc< Matcher, enum_val > )
				{
					if constexpr ( !std::same_as< T, std::monostate > )
						return matcher( tag, *std::get_if< I >( &variant ) );
					else
						return matcher( tag );
				}
				else if constexpr ( HasDefaultMatchFuncNoArg< Matcher > )
				{
					return matcher( std::monostate{} );
				}
				else
				{
					std::unreachable();
				}
			};
		}

		template < typename Matcher, std::size_t... Is >
		static constexpr auto MakeDispatchTable( std::index_sequence< Is... > )
		{
			using CommonReturnType = typename decltype( TryComputeDispatchReturnType< Matcher >( std::index_sequence< Is... >{} ) )::type;
			using FnType = CommonReturnType ( * )( const Matcher&, const StorageType& );

			return std::array< FnType, sizeof...( Is ) >{
				MakeDispatchEntry< Is, Matcher, CommonReturnType >()...
			};
		}

		enum class MatchHandlerError
		{
			None,
			NotMatchCaseOrDefault,
			EmptyCaseMustTakeNoArgs,
			PayloadTypeMismatch,
			DefaultMustTakeNoArgs
		};

		enum class MakeError
		{
			None,
			EmptyCaseGivenArgs,
			PayloadNotConstructible
		};

	protected:
		template < Enum Element, typename... Args >
		constexpr SmartEnumImpl( EnumTag< Element >, Args&&... args )
			: mValueData( std::in_place_index_t< CaseIndex< Element > >{}, std::forward< Args >( args )... )
		{}

	protected:
		template < typename... Handlers >
		constexpr decltype( auto ) MatchImpl( Handlers&&... handlers ) const
		{
			if constexpr ( sizeof...( Handlers ) == 1 )
			{
				auto&& first = std::get< 0 >( std::forward_as_tuple( std::forward< Handlers >( handlers )... ) );
				using First = std::remove_cvref_t< decltype( first ) >;

				if constexpr ( IsOverload< First >::value )
				{
					constexpr auto HandlerError = ValidateOverloadMatcherHelper< First >::FirstError;

					if constexpr ( HandlerError == MatchHandlerError::NotMatchCaseOrDefault )
					{
						SL_COMPILE_ERROR( SmartEnum::Match, "All handlers must be produced by MatchCase(...) or MatchDefault(...)." );
						return Derived{};
					}
					else if constexpr ( HandlerError == MatchHandlerError::EmptyCaseMustTakeNoArgs )
					{
						SL_COMPILE_ERROR( SmartEnum::Match, "Handler for empty cases must be callable with no arguments." );
						return Derived{};
					}
					else if constexpr ( HandlerError == MatchHandlerError::PayloadTypeMismatch )
					{
						SL_COMPILE_ERROR( SmartEnum::Match, "Handler argument type does not match the payload type for this enum case." );
						return Derived{};
					}
					else if constexpr ( HandlerError == MatchHandlerError::DefaultMustTakeNoArgs )
					{
						SL_COMPILE_ERROR( SmartEnum::Match, "Default handler must be callable with no arguments." );
						return Derived{};
					}
					else
					{
						constexpr bool CoverageValid = ValidateOverloadMatcherHelper< First >::CoverageValid;
						SL_COMPILE_CHECK(
							CoverageValid,
							SmartEnum::Match,
							"Must provide handler for all variants in match"
						);

						return DispatchMatcher( std::forward< decltype( first ) >( first ) );
					}
				}
				else
				{
					constexpr auto HandlerError = GetFirstHandlerError< Handlers... >();

					if constexpr ( HandlerError == MatchHandlerError::NotMatchCaseOrDefault )
					{
						SL_COMPILE_ERROR( SmartEnum::Match, "All handlers must be produced by MatchCase(...) or MatchDefault(...)." );
						return Derived{};
					}
					else if constexpr ( HandlerError == MatchHandlerError::EmptyCaseMustTakeNoArgs )
					{
						SL_COMPILE_ERROR( SmartEnum::Match, "Handler for an empty case must be callable with no arguments." );
						return Derived{};
					}
					else if constexpr ( HandlerError == MatchHandlerError::PayloadTypeMismatch )
					{
						SL_COMPILE_ERROR( SmartEnum::Match, "Handler argument type does not match the payload type for this enum case." );
						return Derived{};
					}
					else if constexpr ( HandlerError == MatchHandlerError::DefaultMustTakeNoArgs )
					{
						SL_COMPILE_ERROR( SmartEnum::Match, "Default handler must be callable with no arguments." );
						return Derived{};
					}
					else
					{
						using Matcher = Overload< std::remove_cvref_t< Handlers >... >;
						constexpr bool CoverageValid = IsMatcherCoverageValid< Matcher >( std::index_sequence_for< Variants... >{} );

						SL_COMPILE_CHECK(
							CoverageValid,
							SmartEnum::Match,
							"Must provide handler for all variants in match"
						);

						auto matcher = Overload{ std::forward< Handlers >( handlers )... };
						return DispatchMatcher( matcher );
					}
				}
			}
			else
			{
				constexpr auto HandlerError = GetFirstHandlerError< Handlers... >();

				if constexpr ( HandlerError == MatchHandlerError::NotMatchCaseOrDefault )
				{
					SL_COMPILE_ERROR( SmartEnum::Match, "All handlers must be produced by MatchCase(...) or MatchDefault(...)." );
					return Derived{};
				}
				else if constexpr ( HandlerError == MatchHandlerError::EmptyCaseMustTakeNoArgs )
				{
					SL_COMPILE_ERROR( SmartEnum::Match, "Handler for an empty case must be callable with no arguments." );
					return Derived{};
				}
				else if constexpr ( HandlerError == MatchHandlerError::PayloadTypeMismatch )
				{
					SL_COMPILE_ERROR( SmartEnum::Match, "Handler argument types do not match the payload type for all enum cases." );
					return Derived{};
				}
				else if constexpr ( HandlerError == MatchHandlerError::DefaultMustTakeNoArgs )
				{
					SL_COMPILE_ERROR( SmartEnum::Match, "Default handler must be callable with no arguments." );
					return Derived{};
				}
				else
				{
					using Matcher = Overload< std::remove_cvref_t< Handlers >... >;
					constexpr bool CoverageValid = IsMatcherCoverageValid< Matcher >( std::index_sequence_for< Variants... >{} );

					SL_COMPILE_CHECK(
						CoverageValid,
						SmartEnum::Match,
						"Must provide handler for all variants in match"
					);

					auto matcher = Overload{ std::forward< Handlers >( handlers )... };
					return DispatchMatcher( matcher );
				}
			}
		}

		template < Enum Element, typename... Args >
		static constexpr auto MakeImpl( Args&&... args )
		{
			constexpr auto Error = GetMakeError< Element, Args... >();

			if constexpr ( Error == MakeError::None )
			{
				return Derived( EnumTag< Element >{}, std::forward< Args >( args )... );
			}
			else if constexpr ( Error == MakeError::EmptyCaseGivenArgs )
			{
				SL_COMPILE_ERROR(
					SmartEnum::Make,
					"This enum case does not store a value, so no arguments may be provided."
				);
				return Derived{};
			}
			else if constexpr ( Error == MakeError::PayloadNotConstructible )
			{
				SL_COMPILE_ERROR(
					SmartEnum::Make,
					"The payload for this enum case cannot be constructed from the arguments provided."
				);
				return Derived{};
			}
		}

		constexpr Enum GetEnumImpl() const
		{
			return EnumValues[ mValueData.index() ];
		}

		template < Enum Element, typename Self >
		constexpr decltype( auto ) GetValueImpl( this Self&& self )
		{
			static_assert( HasType< Element >, "Element type must not be empty to unwrap" );
			return std::get< CaseIndex< Element > >( std::forward< Self >( self ).mValueData );
		}

	private:
		template < Enum Element, typename... Args >
		static consteval MakeError GetMakeError()
		{
			using T = ValueTypeAt< Element >;

			if constexpr ( std::same_as< T, std::monostate > )
			{
				if constexpr ( sizeof...( Args ) == 0 )
					return MakeError::None;
				else
					return MakeError::EmptyCaseGivenArgs;
			}
			else
			{
				if constexpr ( std::constructible_from< T, Args... > )
					return MakeError::None;
				else
					return MakeError::PayloadNotConstructible;
			}
		}

		template < typename Handler >
		static consteval MatchHandlerError GetHandlerError()
		{
			using CleanHandler = std::remove_cvref_t< Handler >;

			if constexpr ( IsRelevantEnumMatchCaseHandlerV< CleanHandler, Enum > )
			{
				static constexpr auto Element = CleanHandler::Element;
				using TargetPayload = ValueTypeAt< Element >;

				if constexpr ( std::same_as< TargetPayload, std::monostate > )
				{
					if constexpr ( std::invocable< CleanHandler, EnumTag< Element > > )
						return MatchHandlerError::None;
					else
						return MatchHandlerError::EmptyCaseMustTakeNoArgs;
				}
				else
				{
					if constexpr ( std::invocable< CleanHandler, EnumTag< Element >, TargetPayload const& > )
						return MatchHandlerError::None;
					else
						return MatchHandlerError::PayloadTypeMismatch;
				}
			}
			else if constexpr ( IsDefaultMatchCaseHandler< CleanHandler >::value )
			{
				if constexpr ( std::invocable< CleanHandler, std::monostate > )
					return MatchHandlerError::None;
				else
					return MatchHandlerError::DefaultMustTakeNoArgs;
			}
			else if constexpr ( IsEnumMatchCaseHandler< CleanHandler >::value )
			{
				return MatchHandlerError::None;
			}
			else
			{
				return MatchHandlerError::NotMatchCaseOrDefault;
			}
		}

		template < typename... Handlers >
		struct FirstHandlerErrorHelper;

		template <>
		struct FirstHandlerErrorHelper<>
		{
			static constexpr MatchHandlerError Value = MatchHandlerError::None;
		};

		template < typename First, typename... Rest >
		struct FirstHandlerErrorHelper< First, Rest... >
		{
			static constexpr MatchHandlerError Head = GetHandlerError< First >();
			static constexpr MatchHandlerError Value =
				Head == MatchHandlerError::None
					? FirstHandlerErrorHelper< Rest... >::Value
					: Head;
		};

		template < typename... Handlers >
		static consteval MatchHandlerError GetFirstHandlerError()
		{
			return FirstHandlerErrorHelper< std::remove_cvref_t< Handlers >... >::Value;
		}

		template < typename Matcher, Enum Element >
		static consteval bool HasCoverageForElement()
		{
			return HasMatchFunc< Matcher, Element > || HasDefaultMatchFuncNoArg< Matcher >;
		}

		template < typename Matcher, std::size_t... Is >
		static consteval bool IsMatcherCoverageValid( std::index_sequence< Is... > )
		{
			return ( ... && HasCoverageForElement< Matcher, CaseTypeAt< Is >::Value >() );
		}

		template < typename T >
		struct ValidateOverloadMatcherHelper;

		template < typename... Cases >
		struct ValidateOverloadMatcherHelper< Overload< Cases... > >
		{
			static constexpr MatchHandlerError FirstError = FirstHandlerErrorHelper< Cases... >::Value;
			static constexpr bool CoverageValid =
				FirstError == MatchHandlerError::None &&
				IsMatcherCoverageValid< Overload< Cases... > >( std::index_sequence_for< Variants... >{} );
		};

		template < typename Matcher >
		static consteval bool ValidateOverloadMatcher()
		{
			using CleanMatcher = std::remove_cvref_t< Matcher >;
			return ValidateOverloadMatcherHelper< CleanMatcher >::FirstError == MatchHandlerError::None && ValidateOverloadMatcherHelper< CleanMatcher >::CoverageValid;
		}

		template < typename Matcher >
		constexpr decltype( auto ) DispatchMatcher( Matcher&& matcher ) const
		{
			using CleanMatcher = std::remove_cvref_t< Matcher >;
			static constexpr auto DispatchTable =
				MakeDispatchTable< CleanMatcher >( std::index_sequence_for< Variants... >{} );

			auto&& handler = DispatchTable[ mValueData.index() ];
			return std::invoke( handler, matcher, mValueData );
		}

	private:
		StorageType mValueData;
	};

	template < typename T >
	struct IsSmartEnum : std::false_type
	{};

	template < IsEnum Enum, typename... Specs >
	struct IsSmartEnum< sl::SmartEnum< Enum, Specs... > > : std::true_type
	{};

} // namespace sl::detail
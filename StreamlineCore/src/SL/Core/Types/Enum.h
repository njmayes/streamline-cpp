#pragma once

#include "SL/Core/Common/Base.h"

#include "Detail/Enum.h"

namespace sl {

	template < auto E, typename F >
	detail::EnumMatchCaseHandler< E, F > MatchCase( F&& f )
	{
		return detail::EnumMatchCaseHandler< E, F >{ std::forward< F >( f ) };
	}

	template < typename F >
	detail::EnumDefaultMatchCaseHandler< F > MatchDefault( F&& f )
	{
		return detail::EnumDefaultMatchCaseHandler< F >{ std::forward< F >( f ) };
	}

	inline detail::EnumDefaultMatchCaseHandler< void ( * )() > MatchDefault()
	{
		static auto NoOp = +[] {};
		return detail::EnumDefaultMatchCaseHandler< void ( * )() >{ NoOp };
	}

	template < auto Enum, typename Underlying = std::monostate >
		requires std::is_enum_v< decltype( Enum ) >
	struct Case
	{
		static constexpr auto Tag = detail::EnumTag< Enum >{};
		static constexpr auto Value = Enum;
		using Type = Underlying;
	};

	/*	Usage example:

		enum class TestEnum
		{
			OutOfBounds,
			Unexpected,
			Other
		};

		using SmartTestEnum = sl::SmartEnum< TestEnum,
			sl::Case< TestEnum::OutOfBounds >,
			sl::Case< TestEnum::Unexpected, std::string_view >,
			sl::Case< TestEnum::Other >
		>;

		SmartTestEnum foo = SmartTestEnum::Make< TestEnum::Unexpected >( "Actual value" );

		auto bar = asasfa.Match(
			sl::MatchCase< ErrorEnum::OutOfBounds >( [] { return "OutOfBounds"; } ),
			sl::MatchCase< ErrorEnum::Unexpected >( []( std::string_view value ) { return value; } ),
			sl::MatchDefault( [] { return "Default"; }
		);
	*/

	template < IsEnum Enum, typename... Cases >
	class SmartEnum
	{
	private:
		template < typename T >
		struct IsCase : std::false_type
		{};

		template < auto E, typename U >
		struct IsCase< Case< E, U > > : std::true_type
		{};

		static consteval bool AllCasesUniqueAndValid()
		{
			constexpr bool AllValidCases = ( ... and IsCase< Cases >::value );
			if constexpr ( not AllValidCases )
			{
				static_assert( false, "All types must be Case<E, Underlying> specializations" );
				return false;
			}

			constexpr auto CaseValues = std::array{ Cases::Value... };
			constexpr auto EnumValues = magic_enum::enum_values< Enum >();

			constexpr auto CaseCounts = [ = ] {
				std::array< std::size_t, magic_enum::enum_count< Enum >() > counts{}; // size = EnumValues.size()
				for ( std::size_t i = 0; i < EnumValues.size(); ++i )
					counts[ i ] = std::ranges::count( CaseValues, EnumValues[ i ] );
				return counts;
			}();

			constexpr auto ZeroCount = [ = ]( std::size_t count ) {
				return count == 0;
			};

			constexpr auto MultipleCount = [ = ]( std::size_t count ) {
				return count > 1;
			};

			if constexpr ( std::ranges::any_of( CaseCounts, ZeroCount ) )
			{
				static_assert( false, "SmartEnum must provide case for every enum element" );
				return false;
			}

			if constexpr ( std::ranges::any_of( CaseCounts, MultipleCount ) )
			{
				static_assert( false, "SmartEnum must provide only one case for each enum element" );
				return false;
			}

			return true;
		}

		static_assert( AllCasesUniqueAndValid(), "SmartEnum cases do not satisfy conditions" );

		using CaseTypes = TypeList< Cases... >;
		using ValueTypes = TypeList< typename Cases::Type... >;

		using StorageType = ValueTypes::VariantType;

		static constexpr auto EnumElements = std::array< Enum, sizeof...( Cases ) >{ Cases::Value... };

		template < Enum Element >
		static consteval auto FindEnumIndex()
		{
			auto it = std::find( EnumElements.begin(), EnumElements.end(), Element );
			return std::distance( EnumElements.begin(), it );
		}

		template < size_t I >
		using CaseTypeAt = typename CaseTypes::template Type< I >;

		template < Enum Element >
		static constexpr auto CaseIndex = FindEnumIndex< Element >();

		template < Enum Element >
		using ValueTypeAt = typename ValueTypes::template Type< CaseIndex< Element > >;

		template < Enum Element >
		static constexpr bool HasType = !( std::same_as< ValueTypeAt< Element >, std::monostate > );

		template < typename R >
		static constexpr bool ContainsType = ValueTypes::template Contains< R >;

		template < typename F, Enum Element >
		static consteval bool ComputeHasMatchFunc()
		{
			if constexpr ( HasType< Element > )
				return std::invocable< F, detail::EnumTag< Element >, const ValueTypeAt< Element >& >;
			else
				return std::invocable< F, detail::EnumTag< Element > >;
		}
		template < typename F, Enum Element >
		static consteval bool ComputeHasDefaultMatchFuncWithArg()
		{
			using DefaultHandler = typename detail::ExtractDefaultHandler< F >::type;
			if constexpr ( !std::is_void_v< DefaultHandler > )
			{
				using Callable = decltype( std::declval< DefaultHandler >().func );
				return std::invocable< Callable, const ValueTypeAt< Element >& >;
			}
			else
			{
				return false;
			}
		}
		template < typename F, Enum Element >
		static consteval bool ComputeHasDefaultMatchFuncNoArg()
		{
			using DefaultHandler = typename detail::ExtractDefaultHandler< F >::type;
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

		template < typename F, Enum Element >
		static constexpr bool HasDefaultMatchFuncWithArg = ComputeHasDefaultMatchFuncWithArg< F, Element >();

		template < typename F, Enum Element >
		static constexpr bool HasDefaultMatchFuncNoArg = ComputeHasDefaultMatchFuncNoArg< F, Element >();

		template < typename... Ts >
		struct HasCommonTypeDetail
		{
		private:
			template < typename... Us >
			static auto Test( int ) -> decltype( void( typename std::common_type< Us... >::type{} ), std::true_type{} );

			template < typename... >
			static auto Test( ... ) -> std::false_type;

		public:
			static constexpr bool Value = decltype( Test< Ts... >( 0 ) )::value;
		};

		template < typename... Ts >
		static constexpr bool HasCommonType = HasCommonTypeDetail< Ts... >::Value;

		template < std::size_t I, typename Matcher >
		static consteval decltype( auto ) SimulateDispatchEntry()
		{
			using Case = CaseTypeAt< I >;
			using T = typename Case::Type;

			// unary + forces decay to function pointer
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
				else if constexpr ( HasDefaultMatchFuncWithArg< Matcher, enum_val > )
				{
					return matcher( std::monostate{}, *std::get_if< I >( &variant ) );
				}
				else if constexpr ( HasDefaultMatchFuncNoArg< Matcher, enum_val > )
				{
					return matcher( std::monostate{} );
				}
				else
				{
					static_assert( false, "Must provide handler for all cases in match" );
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
			static constexpr bool HandlersHaveCommonType =
				HasCommonType< decltype( SimulateDispatchEntry< Is, Matcher >()( std::declval< Matcher >(), std::declval< StorageType >() ) )... >;

			static_assert( HandlersHaveCommonType, "Match case handlers must return the same (or implicitly convertible) type for all cases." );

			if constexpr ( not HandlersHaveCommonType )
				return std::type_identity< void >{};
			else
				return ComputeDispatchReturnType< Matcher >( std::index_sequence< Is... >{} );
		}

		template < std::size_t I, typename Matcher, typename ReturnType >
		static constexpr decltype( auto ) MakeDispatchEntry()
		{
			using Case = CaseTypeAt< I >;
			using T = typename Case::Type;

			// unary + forces decay to function pointer
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
				else if constexpr ( HasDefaultMatchFuncWithArg< Matcher, enum_val > )
				{
					return matcher( std::monostate{}, *std::get_if< I >( &variant ) );
				}
				else if constexpr ( HasDefaultMatchFuncNoArg< Matcher, enum_val > )
				{
					return matcher( std::monostate{} );
				}
				else
				{
					static_assert( false, "Must provide handler for all cases in match" );
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

	public:
		// If return types of Handlers... are ReturnType..., then return type of Match is std::common_type_t< ReturnType... >
		template < typename... Handlers >
		constexpr decltype( auto ) Match( Handlers&&... handlers ) const
		{
			auto matcher = detail::Overload{ std::forward< Handlers >( handlers )... };
			auto table = MakeDispatchTable< decltype( matcher ) >( std::index_sequence_for< Cases... >{} );

			return table[ mValueData.index() ]( matcher, mValueData );
		}

		template < Enum Element, typename... Args >
			requires std::constructible_from< ValueTypeAt< Element >, Args... >
		static constexpr auto Make( Args&&... args )
		{
			return SmartEnum( detail::EnumTag< Element >{}, std::forward< Args >( args )... );
		}

		template < Enum Element, typename Self >
		constexpr decltype( auto ) GetValue( this Self&& self )
		{
			static_assert( HasType< Element >, "Element type must not be empty to unwrap" );
			constexpr auto Index = detail::EnumTag< Element >::Index;

			return std::get< Index >( std::forward< Self >( self ).mValueData );
		}

		constexpr Enum GetEnum() const
		{
			return EnumElements[ mValueData.index() ];
		}

	private:
		template < Enum Element, typename... Args >
		constexpr SmartEnum( detail::EnumTag< Element > tag, Args&&... args )
			: mValueData( std::in_place_index_t< CaseIndex< Element > >{}, std::forward< Args >( args )... )
		{}

	private:
		StorageType mValueData;
	};


	namespace detail {

		template < typename T >
		struct IsSmartEnumDetail : std::false_type
		{};

		template < IsEnum Enum, typename... Cases >
		struct IsSmartEnumDetail< SmartEnum< Enum, Cases... > > : std::true_type
		{};

		template < typename T >
		concept IsSmartEnum = IsSmartEnumDetail< T >::value;

	} // namespace detail

} // namespace sl
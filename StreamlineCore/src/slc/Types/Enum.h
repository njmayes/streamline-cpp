#pragma once

#include "slc/Common/Base.h"

namespace slc {

	namespace detail {
		template < typename... Ts >
		concept ValidEnumTypes = ( ( not std::is_pointer_v< Ts > and std::same_as< std::remove_cvref_t< Ts >, Ts > ) and ... );

		template < auto E >
			requires std::is_scoped_enum_v< decltype( E ) >
		struct EnumTag : std::integral_constant< decltype( E ), E >
		{
			SCONSTEXPR auto Index = std::to_underlying( E );
		};

		template < auto E, typename F >
		struct EnumMatchCaseHandler
		{
			F func;

			template < typename... Args >
			auto operator()( EnumTag< E >, Args&&... args ) const
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

	template < auto E, typename Underlying = std::monostate >
	struct Case
	{
		static constexpr auto Tag = detail::EnumTag< E >{};
		static constexpr auto Value = E;
		using Type = Underlying;
	};

	namespace detail {

		template < typename T >
		struct IsCase : std::false_type
		{};

		template < auto E, typename U >
		struct IsCase< Case< E, U > > : std::true_type
		{};

		template < typename T >
		concept EnumCase = IsCase< T >::value;

	} // namespace detail


	/*	Usage example:

		enum class TestEnum
		{
			OutOfBounds,
			Unexpected
		};

		using SmartTestEnum = slc::SmartEnum< TestEnum,
			slc::Case< TestEnum::OutOfBounds >,
			slc::Case< TestEnum::Unexpected, std::string_view >
		>;

		SmartTestEnum foo = SmartTestEnum::Make< TestEnum::Unexpected >( "Actual value" );

		auto bar = asasfa.Match(
			slc::MatchCase< ErrorEnum::OutOfBounds >( [] { return "OutOfBounds"; } ),
			slc::MatchCase< ErrorEnum::Unexpected >( []( std::string_view value ) { return value; } )
		);
	*/

	template < IsEnum Enum, typename... Cases >
	class SmartEnum
	{
	private:
		static_assert( ( ... and detail::EnumCase< Cases > ), "All types must be Case<E, Underlying> specializations" );

		using CaseTypes = TypeList< Cases... >;
		using ValueTypes = TypeList< typename Cases::Type... >;

		using StorageType = ValueTypes::VariantType;

	private:
		template < size_t I >
		using CaseTypeAt = typename CaseTypes::template Type< I >;

		template < Enum Element >
		static constexpr auto CaseIndex = magic_enum::enum_index( Element ).value();

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
		static constexpr auto TryComputeDispatchReturnType( std::index_sequence< Is... > )
		{
			using SimulatedTypes = std::tuple<
				decltype( SimulateDispatchEntry< Is, Matcher >()( std::declval< Matcher >(), std::declval< StorageType >() ) )... >;

			using RawReturnTypes = std::tuple_element_t< 0, SimulatedTypes >; // just for error messages if needed

			static_assert(
				HasCommonType< decltype( SimulateDispatchEntry< Is, Matcher >()( std::declval< Matcher >(), std::declval< StorageType >() ) )... >,
				"Match case handlers must return the same (or implicitly convertible) type for all cases."
			);

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
		decltype( auto ) Match( Handlers&&... handlers ) const
		{
			auto matcher = slc::detail::Overload{ std::forward< Handlers >( handlers )... };
			const auto table = MakeDispatchTable< decltype( matcher ) >( std::index_sequence_for< Cases... >{} );

			return table[ mValueData.index() ]( matcher, mValueData );
		}

		template < Enum Element, typename... Args >
			requires std::constructible_from< ValueTypeAt< Element >, Args... >
		static constexpr auto Make( Args&&... args )
		{
			return SmartEnum( detail::EnumTag< Element >{}, std::forward< Args >( args )... );
		}
		
		template < Enum Element, typename Self >
		decltype( auto ) Unwrap( this Self&& self )
		{
			return *std::get_if< ::slc::detail::EnumTag< Element >::Index >( std::addressof( std::forward< Self >( self ).mValueData ) );
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

		template < IsEnum Enum, typename... Cases  >
		struct IsSmartEnumDetail< SmartEnum< Enum, Cases... > > : std::true_type
		{};

		template < typename T >
		concept IsSmartEnum = IsSmartEnumDetail< T >::value;

	} // namespace detail

} // namespace slc
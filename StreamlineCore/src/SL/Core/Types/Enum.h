#pragma once

#include <SL/Core/Common/Base.h>
#include <SL/Core/Types/Detail/Enum.h>

namespace sl {

	template < auto E, typename T = std::monostate >
	using Case = detail::Variant< E, T >;

	template < auto E, typename F >
	constexpr auto MatchCase( F&& func )
	{
		return detail::EnumMatchCaseHandler< E, std::remove_cvref_t< F > >{
			std::forward< F >( func )
		};
	}

	template < typename F >
	constexpr auto MatchDefault( F&& func )
	{
		return detail::EnumDefaultMatchCaseHandler< std::remove_cvref_t< F > >{
			std::forward< F >( func )
		};
	}

	inline constexpr auto MatchDefault()
	{
		return MatchDefault( [] {} );
	}

	

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

		auto bar = foo.Match(
			sl::MatchCase< ErrorEnum::OutOfBounds >( [] { return "OutOfBounds"; } ),
			sl::MatchCase< ErrorEnum::Unexpected >( []( std::string_view value ) { return value; } ),
			sl::MatchDefault( [] { return "Default"; }
		);
	*/

	template < IsEnum Enum, typename... Specs >
	class SmartEnum : public detail::SmartEnumImpl< SmartEnum< Enum, Specs... >, Enum, Specs... >
	{
	private:
		using Base = detail::SmartEnumImpl< SmartEnum< Enum, Specs... >, Enum, Specs... >;
		friend Base;

	public:
		template < Enum E, typename... Args >
		static constexpr auto Make( Args&&... args )
		{
			return Base::template MakeImpl< E >( std::forward< Args >( args )... );
		}

		template < typename... Handlers >
		constexpr decltype( auto ) Match( Handlers&&... handlers ) const
		{
			return this->MatchImpl( std::forward< Handlers >( handlers )... );
		}

		constexpr Enum GetEnum() const
		{
			return this->GetEnumImpl();
		}

		template < Enum E >
		constexpr decltype( auto ) GetValue() &
		{
			return this->template GetValueImpl< E >();
		}

		template < Enum E >
		constexpr decltype( auto ) GetValue() const&
		{
			return this->template GetValueImpl< E >();
		}

		template < Enum E >
		constexpr decltype( auto ) GetValue() &&
		{
			return std::move( this->template GetValueImpl< E >() );
		}

		template < Enum E >
		constexpr decltype( auto ) GetValue() const&&
		{
			return std::move( this->template GetValueImpl< E >() );
		}

	protected:
		constexpr SmartEnum() = default;

		template < Enum E, typename... Args >
		constexpr SmartEnum( detail::EnumTag< E > tag, Args&&... args )
			: Base( tag, std::forward< Args >( args )... )
		{}
	};

} // namespace sl
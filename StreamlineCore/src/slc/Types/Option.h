#pragma once

#include "Result.h"

namespace slc
{

	namespace detail
	{
		SLC_MAKE_SMART_ENUM( NoneEnum, None )
	}

	template < typename T >
	class Option : private Result< T, detail::NoneEnum >
	{
	public:
		SCONSTEXPR detail::NoneEnum None = detail::NoneEnum::None;

	private:
		using BaseType = Result< T, detail::NoneEnum >;

	public:
		SCONSTEXPR auto Some = BaseType::Success;

	public:
		constexpr bool IsSome() const noexcept
		{
			return this->IsOk();
		}
		template < typename Func >
			requires IsPredicate< Func, T const& >
		constexpr bool IsSomeAnd( Func&& func ) const noexcept
		{
			if ( not IsSome() )
				return false;

			return func( this->GetValRef() );
		}

		constexpr bool IsNone() const noexcept
		{
			return this->IsError();
		}

		template < typename Func >
			requires IsPredicate< Func, T const& >
		constexpr bool IsNoneOr( Func&& func ) const noexcept
		{
			if ( IsNone() )
				return true;

			return func( this->GetValRef() );
		}

		/// <summary>
		/// Throws runtime error with a provided custom message or returns result
		/// </summary>
		constexpr T Expect( const std::string_view msg )
		{
			return this->Expect( msg );
		}

		/// <summary>
		/// Throws runtime error with a generic message or returns result
		/// </summary>
		constexpr T Unwrap()
		{
			return this->Unwrap();
		}

		/// <summary>
		/// Returns result or the provided default value
		/// </summary>
		constexpr T UnwrapOr( T&& default_value ) noexcept( BaseType::IsNoExceptCopy && BaseType::IsNoExceptMove )
			requires( BaseType::IsReference or std::copyable< T > )
		{
			return this->UnwrapOr( default_value );
		}
		/// <summary>
		/// Returns result or the default value of the underlying type
		/// </summary>
		constexpr T UnwrapOrDefault() noexcept( BaseType::IsNoExceptDefNew && BaseType::IsNoExceptCopy && BaseType::IsNoExceptMove )
			requires( not BaseType::IsReference and std::is_default_constructible_v< T > )
		{
			return this->UnwrapOrDefault();
		}
		/// <summary>
		/// Returns result or the result of evaluating the provided function
		/// </summary>
		template < typename Func >
			requires IsFunc< Func, T >
		constexpr T UnwrapOrElse( Func&& op ) noexcept( noexcept( op() ) && BaseType::IsNoExceptCopy && BaseType::IsNoExceptMove )
		{
			return UnwrapOrElse( std::forward< Func >( op ) );
		}

		template < IsEnum E >
		Result< T, E > OkOr( E err ) noexcept( BaseType::IsNoExceptMove )
		{
			using ReturnType = Result< T, E >;
			return this->IsOk() ? ReturnType( this->GetVal() ) : ReturnType( err );
		}
		template < IsEnum E >
		Result< T, E > OkOrElse( IsFunc< E > auto&& err ) noexcept( BaseType::IsNoExceptMove )
		{
			using ReturnType = Result< T, E >;
			return this->IsOk() ? ReturnType( this->GetVal() ) : ReturnType( err );
		}

		/// <summary>
		/// Transforms Result&lt;T, E&gt; into Result&lt;R, E&gt; by applying the provided function to the contained value of Ok and leaving Err values unchanged
		/// </summary>
		template < typename U, typename Func >
			requires IsFunc< Func, U&&, T&& >
		constexpr Option< U > Map( Func&& op ) noexcept( noexcept( op( std::declval< T&& >() ) ) && BaseType::IsNoExceptMove && BaseType::IsNoExceptMove )
		{
			return this->Map( std::forward< Func >( op ) );
		}


		/// <summary>
		/// Applies the provided function to the contained value of Ok, or returns the provided default value if the Result is Err.
		/// Function returns U&amp;&amp; where U is a possibly new type.
		/// </summary>
		/// <typeparam name="Func"></typeparam>
		/// <typeparam name="U"></typeparam>
		/// <param name="defaultVal"></param>
		/// <param name="op"></param>
		/// <returns></returns>
		template < typename U, typename Func >
			requires IsFunc< Func, U&&, T&& >
		constexpr U MapOr( U&& default_val, Func&& op ) noexcept( noexcept( op( std::declval< T&& >() ) ) && BaseType::IsNoExceptMove && BaseType::IsNoExceptMove )
		{
			return this->MapOr( std::forward< U >( default_val ), std::forward< Func >( op ) );
		}


		template < typename... Cases >
		void Match( Cases&&... cases )
		{
			return this->Match( std::forward< Cases >( cases )... );
		}
	};

	template < typename T >
	struct SomeFunctor;

	template < typename T >
	struct NoneFunctor;

	template < typename T >
	struct SomeFunctor< Option< T > >
	{
		constexpr Option< T > operator()( T&& result ) const noexcept( noexcept( T( std::forward< T >( std::declval< T >() ) ) ) )
		{
			return Option< T >( std::forward< T >( result ) );
		}

		template < typename... Args >
		constexpr Option< T > operator()( Args&&... args ) const noexcept( Option< T >::IsNoExceptMove && Option< T >::template IsNoExceptNew< Args... > )
		{
			T&& val = T( std::forward< Args >( args )... );
			return Option< T >( std::move( val ) );
		}
	};

	template < typename T >
	struct NoneFunctor< Option< T > >
	{
		constexpr Option< T > operator()() const noexcept
		{
			return Option< T >( Option< T >::None );
		}
	};

	template < typename T >
	SCONSTEXPR SomeFunctor< T > Some;

	template < typename T >
	SCONSTEXPR NoneFunctor< T > None;
} // namespace slc
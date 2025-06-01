#pragma once

#include "Enum.h"

namespace slc {
	template < typename T >
	class Option;

	template < typename T, IsSmartEnum E >
	class Result;

	template < typename Func, typename T >
	concept ReturnsResult = requires( Func&& f, T&& val ) {
		{ std::invoke( std::forward< Func >( f ), std::forward< T >( val ) ) } -> std::convertible_to< Result< typename decltype( std::invoke( std::forward< Func >( f ), std::forward< T >( val ) ) )::ValueType, typename decltype( std::invoke( std::forward< Func >( f ), std::forward< T >( val ) ) )::ErrorEnum > >;
	};

	template < typename T, IsSmartEnum E >
	class Result
	{
	public:
		SCONSTEXPR bool IsReference = std::is_reference_v< T >;

		using RefType = std::conditional_t< IsReference, T, T& >;
		using ValueType = std::remove_reference_t< T >;

		using ResultType = std::conditional_t< IsReference, std::reference_wrapper< ValueType >, T >;

		using Type = Result< T, E >;

		template < typename... Args >
		SCONSTEXPR bool IsNoExceptNew = std::is_nothrow_constructible_v< T, Args... >;

		SCONSTEXPR bool IsNoExceptDefNew = std::is_nothrow_default_constructible_v< T >;
		SCONSTEXPR bool IsNoExceptMove = std::is_nothrow_move_constructible_v< T > && std::is_nothrow_move_assignable_v< T >;
		SCONSTEXPR bool IsNoExceptCopy = std::is_nothrow_copy_constructible_v< T > && std::is_nothrow_copy_assignable_v< T >;

		SLC_MAKE_SMART_ENUM( ResultEnum, ( Success, ResultType ) )

		SCONSTEXPR auto Ok = ResultEnum::Success;

		using ErrorEnum = E;

		using StorageType = std::variant< ResultEnum, ErrorEnum >;

	public:
		explicit constexpr Result() = delete;
		explicit constexpr Result( T&& result ) noexcept( std::is_nothrow_constructible_v< ResultType, T&& > )
			: mValue( ResultEnum( Ok, std::forward< T >( result ) ) ), mResult( true )
		{}

		explicit constexpr Result( E error ) noexcept
			: mValue( error ), mResult( false )
		{}
		virtual ~Result() = default;

		constexpr bool IsOk() const noexcept
		{
			return mResult;
		}
		constexpr bool IsError() const noexcept
		{
			return !mResult;
		}

		/*
			Extracting contained values

			These methods extract the contained value in a Result<T, E> when it is the Ok variant. Error behaviour depends on the method.
			expect() and unwrap() are strongly discouraged and should only be used for unrecoverable errors.
		*/

		/// <summary>
		/// Throws runtime error with a provided custom message or returns result
		/// </summary>
		constexpr T Expect( const std::string_view msg )
		{
			if ( !mResult )
				throw std::runtime_error( msg.data() );

			return MoveVal();
		}

		/// <summary>
		/// Throws runtime error with a generic message or returns result
		/// </summary>
		constexpr T Unwrap()
		{
			return Expect( "emergency failure" );
		}

		/// <summary>
		/// Returns result or the provided default value
		/// </summary>
		constexpr T UnwrapOr( T&& defaultValue ) noexcept( IsNoExceptCopy && IsNoExceptMove )
			requires( IsReference or std::copyable< T > )
		{
			if ( !mResult )
				return defaultValue;

			return MoveVal();
		}
		/// <summary>
		/// Returns result or the default value of the underlying type
		/// </summary>
		constexpr T UnwrapOrDefault() noexcept( IsNoExceptDefNew && IsNoExceptCopy && IsNoExceptMove )
			requires( not IsReference and std::is_default_constructible_v< T > )
		{
			return UnwrapOr( T() );
		}
		/// <summary>
		/// Returns result or the result of evaluating the provided function
		/// </summary>
		template < typename Func >
			requires IsFunc< Func, T >
		constexpr T UnwrapOrElse( Func&& op ) noexcept( std::is_nothrow_invocable_v< Func > && IsNoExceptCopy && IsNoExceptMove )
		{
			return UnwrapOr( op() );
		}


		/*
			Transforming contained values

			These methods transform the contained value in a Result<T, E> while maintaining the result.
		*/

		/// <summary>
		/// Transforms Result&lt;T, E&gt; into Result&lt;R, E&gt; by applying the provided function to the contained value of Ok and leaving Err values unchanged
		/// </summary>
		template < typename Func >
			requires std::invocable< Func, T&& >
		constexpr auto Map( Func&& op ) noexcept(
			std::is_nothrow_invocable_v< Func, T&& > &&
			Result< std::invoke_result_t< Func, T&& >, E >::IsNoExceptMove &&
			IsNoExceptMove
		)
		{
			using U = std::invoke_result_t< Func, T&& >;

			if ( mResult )
				return Result< U, E >( std::invoke( std::forward< Func >( op ), MoveVal() ) );
			else
				return Result< U, E >( GetError() );
		}
		/// <summary>
		/// Transforms Result&lt;T, E&gt; into Result&lt;T, O&gt; by applying the provided function to the contained value of Err and leaving Ok values unchanged
		/// </summary>
		template < typename Func >
			requires std::invocable< Func, E > and IsSmartEnum< std::invoke_result_t< Func, E > >
		constexpr auto MapError( Func&& op ) noexcept(
			std::is_nothrow_invocable_v< Func, E > &&
			IsNoExceptMove
		)
		{
			using O = std::invoke_result_t< Func, E >;

			if ( mResult )
				return Result< T, O >( MoveVal() );
			else
				return Result< T, O >( std::invoke( std::forward< Func >( op ), GetError() ) );
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
		template < typename Func, typename Default >
			requires std::invocable< Func, T&& >
		constexpr auto MapOr( Default&& default_val, Func&& op ) noexcept(
			std::is_nothrow_invocable_v< Func, T&& > &&
			IsNoExceptMove
		)
		{
			using U = std::invoke_result_t< Func, T&& >;

			if ( mResult )
				return std::invoke( std::forward< Func >( op ), MoveVal() );
			else
				return static_cast< U >( std::forward< Default >( default_val ) );
		}

		/// <summary>
		/// Applies the provided function to the contained value of Ok, or applies the provided default fallback function to the contained value of Err.
		/// Both functions return U&amp;&amp; where U is a possibly new type.
		/// </summary>
		template < typename Func, typename ErrFunc >
			requires std::invocable< Func, T&& > && std::invocable< ErrFunc, E >
		constexpr auto MapOrElse( Func&& op, ErrFunc&& err_op ) noexcept(
			std::is_nothrow_invocable_v< Func, T&& > &&
			std::is_nothrow_invocable_v< ErrFunc, E > &&
			IsNoExceptMove
		)
		{
			using U = std::invoke_result_t< Func, T&& >;
			static_assert( std::is_same_v< U, std::invoke_result_t< ErrFunc, E > >, "op and err_op must return the same type" );

			if ( mResult )
				return std::invoke( std::forward< Func >( op ), MoveVal() );
			else
				return std::invoke( std::forward< ErrFunc >( err_op ), GetError() );
		}

		/// <summary>
		/// Returns the result of the provided function, or the error result
		/// </summary>
		template < typename Func >
			requires ReturnsResult< Func, T >
		constexpr auto AndThen( Func&& next ) noexcept( std::is_nothrow_invocable_v< Func, T&& > && IsNoExceptMove )
		{
			if ( mResult )
				return next( MoveVal() );

			using Ret = std::invoke_result_t< Func, T >;
			return Ret( GetError() );
		}

		template < typename... Cases >
		void Match( Cases&&... cases )
		{
			auto matcher = ::slc::detail::Overload{ std::forward< Cases >( cases )... };
			using Matcher = decltype( matcher );

			if ( mResult )
			{
				auto const& success = GetSuccessEnum();
				success.Match( std::forward< Matcher >( matcher ) );
			}
			else
			{
				auto const& error = GetError();
				error.Match( std::forward< Matcher >( matcher ) );
			}
		}


		/// <summary>
		/// Returns the option if it contains a value, otherwise returns the provided option
		/// </summary>
		constexpr Result< T, E > const& Or( Result< T, E > const& or_result ) noexcept
		{
			if ( mResult )
				return *this;

			return or_result;
		}

		/// <summary>
		/// Returns the option if it contains a value, otherwise returns the provided option
		/// </summary>
		template < typename Func >
			requires IsFunc< Func, Result< T, E > >
		constexpr Result< T, E > const& OrElse( Func&& func ) noexcept( std::is_nothrow_invocable_v< Func > )
		{
			if ( mResult )
				return *this;

			return func();
		}

	protected:
		/// <summary>
		/// Only for internal use. Should never be used without checking the internal state prior first. Marked noexcept given this assumption as std::get_if should never return null.
		/// </summary>
		/// <returns></returns>
		constexpr T&& MoveVal() noexcept( IsNoExceptMove )
		{
			if constexpr ( IsReference )
				return GetValRef();
			else
				return std::move( GetValRef() );
		}

		/// <summary>
		/// Only for internal use. Should never be used without checking the internal state prior first. Marked noexcept given this assumption as std::get_if should never return null.
		/// </summary>
		/// <returns></returns>
		constexpr RefType GetValRef() noexcept
		{
			return GetSuccessEnum().Unwrap( Ok );
		}
		constexpr const RefType GetValRef() const noexcept
		{
			return GetSuccessEnum().Unwrap( Ok );
		}

		constexpr ResultEnum& GetSuccessEnum() noexcept
		{
			return *std::get_if< ResultEnum >( &mValue );
		}

		constexpr ResultEnum const& GetSuccessEnum() const noexcept
		{
			return *std::get_if< ResultEnum >( &mValue );
		}

		/// <summary>
		/// Only for internal use. Should never be used without checking the internal state prior first. Marked noexcept given this assumption as std::get_if should never return null.
		/// </summary>
		/// <returns></returns>
		constexpr E const& GetError() const noexcept
		{
			return *std::get_if< ErrorEnum >( &mValue );
		}

	private:
		bool mResult{};
		StorageType mValue;
	};

	template < typename T >
	struct OkFunctor;

	template < typename T >
	struct ErrorFunctor;

	template < typename T, IsSmartEnum E >
	struct OkFunctor< Result< T, E > >
	{
		template < typename... Args >
		constexpr Result< T, E > operator()( Args&&... args ) const noexcept( Result< T, E >::IsNoExceptMove && Result< T, E >::template IsNoExceptNew< Args... > )
		{
			T&& val = T( std::forward< Args >( args )... );
			return Result< T, E >( std::move( val ) );
		}
	};

	template < typename T, IsSmartEnum E >
	struct ErrorFunctor< Result< T, E > >
	{
		constexpr Result< T, E > operator()( E error ) const noexcept
		{
			return Result< T, E >( error );
		}

		template < auto O, typename... Args >
		constexpr Result< T, E > operator()( ::slc::detail::EnumTag< O > error, Args&&... args ) const noexcept
		{
			return Result< T, E >( E( error, std::forward< Args >( args )... ) );
		}
	};

	template < typename T >
	constexpr OkFunctor< T > Ok;

	template < typename T >
	constexpr ErrorFunctor< T > Err;

} // namespace slc
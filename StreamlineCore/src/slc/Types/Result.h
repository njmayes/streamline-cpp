#pragma once

#include "slc/Common/Functional.h"

#include "Enum.h"

namespace slc
{
	template < typename T >
	class Option;

	template < typename T, IsRustEnum E >
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
		explicit constexpr Result( T&& result ) noexcept( noexcept( ResultType( std::forward< T >( std::declval< T >() ) ) ) )
			: mValue( ResultEnum( Ok, std::forward< T >( result ) ) ), mResult( true )
		{}

		explicit constexpr Result( E error ) noexcept
			: mValue( error ), mResult( false )
		{}
		virtual ~Result() = default;

		/// <summary>
		/// Converts from const Result<T, E> to Result<const T&, E>
		/// </summary>
		constexpr Result< const T&, E > AsConst() const noexcept
		{
			return mResult ? Result< const T&, E >( GetValRef() ) : Result< const T&, E >( GetError() );
		}

		/// <summary>
		/// Converts from Result<T, E> to Result<T&, E>
		/// </summary>
		constexpr Result< T&, E > AsMutable() noexcept
		{
			return mResult ? Result< T&, E >( GetValRef() ) : Result< T&, E >( GetError() );
		}

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
		constexpr T UnwrapOrElse( Func&& op ) noexcept( noexcept( op() ) && IsNoExceptCopy && IsNoExceptMove )
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
		template < typename U, typename Func >
			requires IsFunc< Func, U&&, T&& >
		constexpr Result< U, E > Map( Func&& op ) noexcept( noexcept( op( std::declval< T&& >() ) ) && Result< U, E >::IsNoExceptMove && IsNoExceptMove )
		{
			if ( mResult )
				return Result< U, E >( op( MoveVal() ) );

			return Result< U, E >( GetError() );
		}
		/// <summary>
		/// Transforms Result&lt;T, E&gt; into Result&lt;T, O&gt; by applying the provided function to the contained value of Err and leaving Ok values unchanged
		/// </summary>
		template < IsRustEnum O, typename Func >
			requires IsFunc< Func, O, E >
		constexpr Result< T, O > MapError( Func&& op ) noexcept( noexcept( op( std::declval< E >() ) ) && IsNoExceptMove )
		{
			if ( mResult )
				return Result< T, O >( MoveVal() );

			return Result< T, O >( op( GetError() ) );
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
		constexpr U MapOr( U&& defaultVal, Func&& op ) noexcept( noexcept( op( std::declval< T&& >() ) ) && Result< U, E >::IsNoExceptMove && IsNoExceptMove )
		{
			if ( mResult )
				return op( MoveVal() );

			return std::move( defaultVal );
		}

		/// <summary>
		/// Applies the provided function to the contained value of Ok, or applies the provided default fallback function to the contained value of Err.
		/// Both functions return U&amp;&amp; where U is a possibly new type.
		/// </summary>
		template < typename U, typename Func, typename ErrFunc >
			requires IsFunc< Func, U&&, T&& > and IsFunc< ErrFunc, U&&, E >
		constexpr U MapOrElse( Func&& op, ErrFunc&& err_op ) noexcept(
			noexcept( op( std::declval< T&& >() ) ) &&
			noexcept( err_op( std::declval< E >() ) ) &&
			Result< U, E >::IsNoExceptMove &&
			IsNoExceptMove )
		{
			if ( mResult )
				return op( MoveVal() );

			return err_op( GetError() );
		}

		/// <summary>
		/// Returns the result of the provided function, or the error result
		/// </summary>
		template < typename Func >
			requires IsFunc< Func, Result< T, E > >
		constexpr Result< T, E > operator|( Func&& next ) noexcept( noexcept( next() ) && IsNoExceptMove )
		{
			if ( mResult )
				return next();

			return Result< T, E >( GetError() );
		}

		/// <summary>
		/// Returns the result of the provided function, or the error result
		/// </summary>
		template < typename Func >
			requires IsFunc< Func, Result< T, E >, T&& >
		constexpr Result< T, E > operator|( Func&& next ) noexcept( noexcept( next( std::declval< T&& >() ) ) && IsNoExceptMove )
		{
			if ( mResult )
				return next( MoveVal() );

			return Result< T, E >( GetError() );
		}

		/// <summary>
		/// Returns the result of the provided function, or the error result
		/// </summary>
		template < typename R, typename Func >
			requires IsFunc< Func, Result< R, E >, T&& >
		constexpr Result< R, E > AndThen( Func&& next ) noexcept( noexcept( next( std::declval< T&& >() ) ) && IsNoExceptMove )
		{
			if ( mResult )
				return next( MoveVal() );

			return Result< R, E >( GetError() );
		}

		template < typename... Cases >
		void Match( Cases&&... cases )
		{
			auto matcher = ::slc::detail::Overload{ std::forward< Cases >( cases )... };
			using Matcher = decltype( matcher );

			if ( mResult )
			{
				auto const& value = GetValRef();
				std::forward< Matcher >( matcher )( Ok, value );
			}
			else
			{
				auto const& error = GetError();
				error.Match( std::forward< Matcher >( matcher ) );
			}
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
			return std::get_if< ResultEnum >( &mValue )->GetValue( Ok );
		}
		constexpr const RefType GetValRef() const noexcept
		{
			return std::get_if< ResultEnum >( &mValue )->GetValue( Ok );
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

	namespace detail
	{

		template < typename T, typename R, IsRustEnum E, typename NextFunc, typename... Func >
			requires IsFunc< NextFunc, Result< T, E >, R&& >
		SCONSTEXPR Result< T, E > DoOperation( Result< R, E > first, NextFunc&& next )
		{
			using FuncReturnType = std::invoke_result_t< NextFunc, R&& >;
			using ResultValueType = FuncReturnType::ResultType;

			return first.AndThen< ResultValueType >( next );
		}

		template < typename T, typename R, IsRustEnum E, typename NextFunc, typename... Func >
			requires IsFunc< NextFunc, Result< T, E >, R&& >
		SCONSTEXPR Result< T, E > DoOperation( Result< R, E > first, NextFunc&& next, Func&&... ops )
		{
			using FuncReturnType = std::invoke_result_t< NextFunc, R&& >;
			using ResultValueType = FuncReturnType::ResultType;

			return DoOperation< ResultValueType, T, E >( first.AndThen< ResultValueType >( next ), std::forward< Func >( ops )... );
		}
	} // namespace detail

	template < typename T, typename R, IsRustEnum E, typename... Func >
	SCONSTEXPR Result< T, E > Do( Result< R, E > first, Func&&... ops )
	{
		return detail::DoOperation< T, R, E >( first, std::forward< Func >( ops )... );
	}


	template < typename T >
	struct OkFunctor;

	template < typename T >
	struct ErrorFunctor;

	template < typename T, IsRustEnum E >
	struct OkFunctor< Result< T, E > >
	{
		constexpr Result< T, E > operator()( T&& result ) const noexcept( noexcept( T( std::forward< T >( std::declval< T >() ) ) ) )
		{
			return Result< T, E >( std::forward< T >( result ) );
		}

		template < typename... Args >
		constexpr Result< T, E > operator()( Args&&... args ) const noexcept( Result< T, E >::IsNoExceptMove && Result< T, E >::template IsNoExceptNew< Args... > )
		{
			T&& val = T( std::forward< Args >( args )... );
			return Result< T, E >( std::move( val ) );
		}
	};

	template < typename T, IsRustEnum E >
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
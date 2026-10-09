#pragma once

#include "Enum.h"

namespace sl {

	template < typename T >
	class Option;

	template < typename T, typename E >
	class Result;

	template < typename Func, typename T >
	concept ReturnsResult = requires( Func&& f, T&& val ) {
		{ std::invoke( std::forward< Func >( f ), std::forward< T >( val ) ) }
		-> std::convertible_to<
			Result<
				typename decltype( std::invoke( std::forward< Func >( f ), std::forward< T >( val ) ) )::ValueType,
				typename decltype( std::invoke( std::forward< Func >( f ), std::forward< T >( val ) ) )::ErrorType > >;
	};

	template < typename T, typename E >
	class Result
	{
	public:
		static constexpr bool IsReference = std::is_reference_v< T >;

		using ValueType = std::remove_reference_t< T >;
		using RefType = std::conditional_t< IsReference, T, T& >;
		using ResultType = std::conditional_t< IsReference, std::reference_wrapper< ValueType >, T >;

		using Type = Result< T, E >;
		using ErrorType = E;

		template < typename... Args >
		static constexpr bool IsValueNoExceptNew = std::is_nothrow_constructible_v< T, Args... >;

		static constexpr bool IsValueNoExceptDefNew = std::is_nothrow_default_constructible_v< T >;
		static constexpr bool IsValueNoExceptMove = std::is_nothrow_move_constructible_v< T > && std::is_nothrow_move_assignable_v< T >;
		static constexpr bool IsValueNoExceptCopy = std::is_nothrow_copy_constructible_v< T > && std::is_nothrow_copy_assignable_v< T >;

		template < typename... Args >
		static constexpr bool IsErrorNoExceptNew = std::is_nothrow_constructible_v< E, Args... >;

		static constexpr bool IsErrorNoExceptDefNew = std::is_nothrow_default_constructible_v< E >;
		static constexpr bool IsErrorNoExceptMove = std::is_nothrow_move_constructible_v< E > && std::is_nothrow_move_assignable_v< E >;
		static constexpr bool IsErrorNoExceptCopy = std::is_nothrow_copy_constructible_v< E > && std::is_nothrow_copy_assignable_v< E >;

		static constexpr bool IsErrorTypeSmartEnum = detail::IsSmartEnum< ErrorType >::value;

		enum class ResultEnum
		{
			Success,
			Failure
		};

		using StorageType = SmartEnum<
			ResultEnum,
			Case< ResultEnum::Success, ResultType >,
			Case< ResultEnum::Failure, ErrorType > >;

		static constexpr auto Ok = ResultEnum::Success;
		static constexpr auto Err = ResultEnum::Failure;

	public:
		Result() = delete;

		explicit constexpr Result( T&& result ) noexcept( std::is_nothrow_constructible_v< ResultType, T&& > )
			: mValue( StorageType::template Make< Ok >( std::forward< T >( result ) ) ), mResult( true )
		{}

		explicit constexpr Result( E error ) noexcept
			: mValue( StorageType::template Make< Err >( error ) ), mResult( false )
		{}

		constexpr operator bool() const noexcept
		{
			return mResult;
		}

		constexpr bool IsOk() const noexcept
		{
			return mResult;
		}
		constexpr bool IsError() const noexcept
		{
			return !mResult;
		}

		//========================
		// Unwrap
		//========================

		constexpr T Expect( std::string_view msg )
		{
			if ( !mResult )
				throw std::runtime_error( msg.data() );

			return MoveVal();
		}

		constexpr T Unwrap()
		{
			return Expect( "emergency failure" );
		}

		constexpr T UnwrapOr( T&& defaultValue ) noexcept( IsValueNoExceptCopy && IsValueNoExceptMove )
			requires( IsReference || std::copyable< T > )
		{
			return mResult ? MoveVal() : std::forward< T >( defaultValue );
		}

		constexpr T UnwrapOrDefault() noexcept( IsValueNoExceptDefNew && IsValueNoExceptCopy && IsValueNoExceptMove )
			requires( !IsReference && std::is_default_constructible_v< T > )
		{
			return UnwrapOr( T{} );
		}

		template < typename Func >
			requires IsFunc< Func, T >
		constexpr T UnwrapOrElse( Func&& op )
		{
			return mResult ? MoveVal() : op();
		}

		constexpr E ExpectError( std::string_view msg )
		{
			if ( !mResult )
				throw std::runtime_error( msg.data() );

			return MoveError();
		}

		constexpr E Error()
		{
			return ExpectError( "emergency failure" );
		}

		//========================
		// Map
		//========================

		template < typename Func >
			requires std::invocable< Func, T&& >
		constexpr auto Map( Func&& op )
		{
			using U = std::invoke_result_t< Func, T&& >;

			if ( mResult )
				return Result< U, E >( std::invoke( std::forward< Func >( op ), MoveVal() ) );

			return Result< U, E >( GetErrorRef() );
		}

		template < typename Func >
			requires std::invocable< Func, E >
		constexpr auto MapError( Func&& op )
		{
			using O = std::invoke_result_t< Func, E >;

			if ( mResult )
				return Result< T, O >( MoveVal() );

			return Result< T, O >( std::invoke( std::forward< Func >( op ), GetErrorRef() ) );
		}

		template < typename Func, typename Default >
			requires std::invocable< Func, T&& >
		constexpr auto MapOr( Default&& default_val, Func&& op )
		{
			using U = std::invoke_result_t< Func, T&& >;

			if ( mResult )
				return std::invoke( std::forward< Func >( op ), MoveVal() );

			return static_cast< U >( std::forward< Default >( default_val ) );
		}

		template < typename Func, typename ErrFunc >
			requires std::invocable< Func, T&& > && std::invocable< ErrFunc, E >
		constexpr auto MapOrElse( Func&& op, ErrFunc&& err_op )
		{
			using U = std::invoke_result_t< Func, T&& >;
			static_assert( std::same_as< U, std::invoke_result_t< ErrFunc, E > > );

			if ( mResult )
				return std::invoke( std::forward< Func >( op ), MoveVal() );

			return std::invoke( std::forward< ErrFunc >( err_op ), GetErrorRef() );
		}

		template < typename Func >
			requires ReturnsResult< Func, T >
		constexpr auto AndThen( Func&& next )
		{
			if ( mResult )
				return next( MoveVal() );

			using Ret = std::invoke_result_t< Func, T >;
			return Ret( GetErrorRef() );
		}

		//========================
		// Match
		//========================

		template < typename... Cases >
		decltype( auto ) Match( Cases&&... cases )
		{
			auto matcher = Overload{ std::forward< Cases >( cases )... };

			if ( mResult )
			{
				return matcher( detail::EnumTag< Ok >{}, GetValRef() );
			}
			else
			{
				if constexpr ( IsErrorTypeSmartEnum )
				{
					return GetErrorRef().Match( matcher );
				}
				else
				{
					return matcher( detail::EnumTag< Err >{}, GetErrorRef() );
				}
			}
		}

		//========================
		// Or
		//========================

		constexpr Result const& Or( Result const& other ) const noexcept
		{
			return mResult ? *this : other;
		}

		template < typename Func >
			requires IsFunc< Func, Result >
		constexpr Result OrElse( Func&& func )
		{
			return mResult ? std::move( *this ) : func();
		}

	protected:
		constexpr T&& MoveVal() noexcept( IsValueNoExceptMove )
		{
			if constexpr ( IsReference )
				return GetValRef();
			else
				return std::move( GetValRef() );
		}

		constexpr RefType GetValRef() noexcept
		{
			return mValue.template GetValue< Ok >();
		}

		constexpr const RefType GetValRef() const noexcept
		{
			return mValue.template GetValue< Ok >();
		}

		constexpr E&& MoveError() noexcept( IsErrorNoExceptMove )
		{
			if constexpr ( IsReference )
				return GetErrorRef();
			else
				return std::move( GetErrorRef() );
		}

		constexpr E& GetErrorRef() noexcept
		{
			return mValue.template GetValue< Err >();
		}

		constexpr const E& GetErrorRef() const noexcept
		{
			return mValue.template GetValue< Err >();
		}

	private:
		bool mResult{};
		StorageType mValue;
	};

	namespace detail {

		template < typename T >
		struct OkFunctor;

		template < typename T >
		struct ErrorFunctor;

		template < typename T, typename E >
		struct OkFunctor< Result< T, E > >
		{
			template < typename... Args >
			constexpr Result< T, E > operator()( Args&&... args ) const
			{
				return Result< T, E >( T( std::forward< Args >( args )... ) );
			}
		};

		template < typename T, typename E >
		struct ErrorFunctor< Result< T, E > >
		{
			constexpr Result< T, E > operator()( E error ) const
			{
				return Result< T, E >( error );
			}

			template < auto O, typename... Args >
			constexpr Result< T, E > operator()( detail::EnumTag< O > tag, Args&&... args ) const
			{
				return Result< T, E >( E( tag, std::forward< Args >( args )... ) );
			}
		};

	} // namespace detail

	template < typename T >
	inline constexpr detail::OkFunctor< T > Ok;

	template < typename T >
	inline constexpr detail::ErrorFunctor< T > Err;

} // namespace sl
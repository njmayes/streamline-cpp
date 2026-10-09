#pragma once

#include "CoroutineContext.h"
#include "TaskState.h"

#include <exception>
#include <functional>
#include <type_traits>

namespace sl {

	template < typename T >
	class Awaitable;

	namespace thread {
		template < typename T >
		struct CoroutinePromise;
		template < typename T >
		struct AwaitableValue;
		template < typename T >
		struct AwaitableValue< Awaitable< T > >
		{
			using type = T;
		};
		template < typename T, typename F >
		Awaitable< ContinuationResultT< T, F > > ChainCoroutine( Awaitable< T > previous, F fn );
	} // namespace thread

	template < typename T >
	class Awaitable
	{
	public:
		using promise_type = thread::CoroutinePromise< T >;
		Awaitable() = default;
		Awaitable( Awaitable&& ) = default;
		Awaitable& operator=( Awaitable&& ) = default;
		Awaitable( const Awaitable& ) = delete;
		Awaitable& operator=( const Awaitable& ) = delete;
		[[nodiscard]] bool Valid() const noexcept
		{
			return static_cast< bool >( state_ );
		}
		decltype( auto ) Get()
		{
			if ( !state_ )
				throw std::future_error( std::future_errc::no_state );
			return std::exchange( state_, {} )->future.get();
		}
		auto operator co_await() &&
		{
			if ( !state_ )
				throw std::future_error( std::future_errc::no_state );
			return NestedAwaiter{ std::exchange( state_, {} ) };
		}
		template < typename F >
		auto Then( F&& fn ) && -> Awaitable< thread::ContinuationResultT< T, F > >
		{
			if ( !state_ )
				throw std::future_error( std::future_errc::no_state );
			return thread::ChainCoroutine< T, std::decay_t< F > >( std::move( *this ), std::forward< F >( fn ) );
		}

	private:
		friend struct thread::CoroutinePromise< T >;
		struct NestedAwaiter
		{
			std::shared_ptr< thread::State< T > > state;
			bool await_ready() const
			{
				return state->Ready();
			}
			bool await_suspend( std::coroutine_handle<> handle )
			{
				const auto* context = thread::currentContext;
				thread::Continuation resume = [ handle, context ] {
					if ( context )
						context->schedule( handle );
					else
						handle.resume();
				};
				return state->Register( resume );
			}
			decltype( auto ) await_resume()
			{
				return state->future.get();
			}
		};
		explicit Awaitable( std::shared_ptr< thread::State< T > > state ) : state_( std::move( state ) )
		{}
		std::shared_ptr< thread::State< T > > state_;
	};

	namespace thread {
		template < typename T >
		struct CoroutinePromise
		{
			std::promise< T > promise;
			std::shared_ptr< State< T > > state = std::make_shared< State< T > >( promise.get_future() );
			Awaitable< T > get_return_object()
			{
				return Awaitable< T >( state );
			}
			std::suspend_never initial_suspend() noexcept
			{
				return {};
			}
			struct Final
			{
				std::shared_ptr< State< T > > state;
				bool await_ready() const noexcept
				{
					return false;
				}
				bool await_suspend( std::coroutine_handle<> ) noexcept
				{
					try
					{
						state->Complete();
					}
					catch ( ... )
					{
						std::terminate();
					}
					return false;
				}
				void await_resume() const noexcept
				{}
			};
			Final final_suspend() noexcept
			{
				return { state };
			}
			template < typename U >
			void return_value( U&& value )
			{
				promise.set_value( std::forward< U >( value ) );
			}
			void unhandled_exception()
			{
				promise.set_exception( std::current_exception() );
			}
		};

		template <>
		struct CoroutinePromise< void >
		{
			std::promise< void > promise;
			std::shared_ptr< State< void > > state = std::make_shared< State< void > >( promise.get_future() );
			Awaitable< void > get_return_object()
			{
				return Awaitable< void >( state );
			}
			std::suspend_never initial_suspend() noexcept
			{
				return {};
			}
			struct Final
			{
				std::shared_ptr< State< void > > state;
				bool await_ready() const noexcept
				{
					return false;
				}
				bool await_suspend( std::coroutine_handle<> ) noexcept
				{
					try
					{
						state->Complete();
					}
					catch ( ... )
					{
						std::terminate();
					}
					return false;
				}
				void await_resume() const noexcept
				{}
			};
			Final final_suspend() noexcept
			{
				return { state };
			}
			void return_void()
			{
				promise.set_value();
			}
			void unhandled_exception()
			{
				promise.set_exception( std::current_exception() );
			}
		};

		template < typename T, typename F >
		Awaitable< ContinuationResultT< T, F > > ChainCoroutine( Awaitable< T > previous, F fn )
		{
			using R = ContinuationResultT< T, F >;
			if constexpr ( std::is_void_v< T > )
			{
				co_await std::move( previous );
				if constexpr ( std::is_void_v< R > )
				{
					std::invoke( std::move( fn ) );
					co_return;
				}
				else
					co_return std::invoke( std::move( fn ) );
			}
			else
			{
				if constexpr ( std::is_void_v< R > )
				{
					std::invoke( std::move( fn ), co_await std::move( previous ) );
					co_return;
				}
				else
					co_return std::invoke( std::move( fn ), co_await std::move( previous ) );
			}
		}
	} // namespace thread
} // namespace sl

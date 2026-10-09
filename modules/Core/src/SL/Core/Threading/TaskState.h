#pragma once

#include "CoroutineContext.h"

#include <coroutine>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>

namespace sl::thread {

	using Continuation = std::move_only_function< void() >;

	template < typename T >
	struct State
	{
		explicit State( std::future< T > value ) : future( std::move( value ) )
		{}

		bool Register( Continuation& fn )
		{
			std::scoped_lock lock( mutex );
			if ( completed )
				return false;
			continuation = std::move( fn );
			return true;
		}

		void Complete()
		{
			Continuation fn;
			{
				std::scoped_lock lock( mutex );
				completed = true;
				fn = std::move( continuation );
			}
			if ( fn )
				fn();
		}

		bool Ready() const
		{
			std::scoped_lock lock( mutex );
			return completed;
		}

		std::future< T > future;
		mutable std::mutex mutex;
		Continuation continuation;
		bool completed = false;
	};

	template < typename T >
	struct FutureAwaiter
	{
		std::shared_ptr< State< T > > state;
		bool await_ready() const
		{
			return state->Ready();
		}
		bool await_suspend( std::coroutine_handle<> handle )
		{
			const auto* context = currentContext;
			Continuation fn = [ handle, context ] {
				if ( context )
					context->schedule( handle );
				else
					handle.resume();
			};
			return state->Register( fn );
		}
		decltype( auto ) await_resume()
		{
			return state->future.get();
		}
	};

	template < typename T, typename F >
	struct ContinuationResult
	{
		using type = std::invoke_result_t< F, T >;
	};
	template < typename F >
	struct ContinuationResult< void, F >
	{
		using type = std::invoke_result_t< F >;
	};
	template < typename T, typename F >
	using ContinuationResultT = typename ContinuationResult< T, std::decay_t< F > >::type;

} // namespace sl::thread

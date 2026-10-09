#pragma once

#include "SL/Core/Common/Base.h"

namespace sl {

	namespace thread {

		using Continuation = std::move_only_function< void() >;

		template < typename Result, typename Continuation >
		struct ContinuationResult
		{
			using type = std::invoke_result_t< Continuation, Result >;
		};

		template < typename Continuation >
		struct ContinuationResult< void, Continuation >
		{
			using type = std::invoke_result_t< Continuation >;
		};
	} // namespace thread

	template < typename Result >
	class Task
	{
	public:
		struct State
		{
			explicit State( std::future< Result > value ) : future( std::move( value ) )
			{}

			void SetContinuation( thread::Continuation callback )
			{
				{
					std::scoped_lock lock( mutex );
					if ( !complete )
					{
						continuation = std::move( callback );
						return;
					}
				}
				callback();
			}

			void Complete()
			{
				thread::Continuation callback;
				{
					std::scoped_lock lock( mutex );
					complete = true;
					callback = std::move( continuation );
				}
				if ( callback )
					callback();
			}

			std::future< Result > future;
			std::mutex mutex;
			thread::Continuation continuation;
			bool complete{};
		};

	public:
		Task() = default;
		Task( Task&& ) = default;
		Task& operator=( Task&& ) = default;
		Task( Task const& ) = delete;
		Task& operator=( Task const& ) = delete;

		[[nodiscard]] bool Valid() const noexcept
		{
			return state_ != nullptr;
		}

		// Consumes the result. Do not call after Then().
		decltype( auto ) GetResult()
		{
			if ( !state_ )
				throw std::future_error( std::future_errc::no_state );
			auto state = std::move( state_ );
			return state->future.get();
		}

		// Consumes this handle and schedules the next task when it completes.
		// Register continuations before stopping the pool.
		template < typename Continuation >
		auto Then( Continuation&& continuation )
			-> Task< typename thread::ContinuationResult< Result, std::decay_t< Continuation > >::type >
		{
			using NextResult = typename thread::ContinuationResult< Result, std::decay_t< Continuation > >::type;
			if ( !state_ )
				throw std::future_error( std::future_errc::no_state );

			auto& pool = *pool_;
			auto previous = std::move( state_->future );
			auto next_callable = [ previous = std::move( previous ),
								   continuation = std::forward< Continuation >( continuation ) ]() mutable -> NextResult {
				if constexpr ( std::is_void_v< Result > )
				{
					previous.get();
					return std::invoke( std::move( continuation ) );
				}
				else
				{
					return std::invoke( std::move( continuation ), previous.get() );
				}
			};

			auto [ next, job ] = pool.CreateScheduledTask( std::move( next_callable ) );
			state_->SetContinuation( [ owner = pool_, job = std::move( job ) ]() mutable {
				owner->EnThreadPoolJob( std::move( job ), true );
			} );
			state_.reset();
			return next;
		}

	private:
		friend class ThreadPool;
		Task( ThreadPool* owner, std::shared_ptr< State > state )
			: pool_( owner ), state_( std::move( state ) )
		{}

		ThreadPool* pool_{};
		std::shared_ptr< State > state_;
	};
} // namespace sl
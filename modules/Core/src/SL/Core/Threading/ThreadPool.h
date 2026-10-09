#pragma once

#include "Task.h"
#include "Awaitable.h"
#include "CoroutineContext.h"

#include "SL/Core/Types/Result.h"

namespace sl {

	class ThreadPool
	{
	public:
		struct Args
		{
			// 0 = hardware concurrency.
			// -N = hardware concurrency minus N, at least 1.
			int num_threads = 0;
			bool autostart = true;
		};

		template < typename F, typename... A >
		using TaskResult = std::invoke_result_t<
			std::decay_t< F >,
			std::decay_t< A >... >;

	public:
		ThreadPool();
		explicit ThreadPool( const Args& args );
		~ThreadPool();

		ThreadPool( const ThreadPool& ) = delete;
		ThreadPool& operator=( const ThreadPool& ) = delete;

		ThreadPool( ThreadPool&& ) = delete;
		ThreadPool& operator=( ThreadPool&& ) = delete;

	public:
		void Start( int num_threads = 0 );
		void Stop();

		[[nodiscard]] std::size_t GetThreadCount() const;

		template < typename F, typename... A >
		auto Submit( F&& function, A&&... args )
			-> Task< TaskResult< F, A... > >
		{
			auto callable = [ fn = std::forward< F >( function ),
							  ... values = std::forward< A >( args ) ] mutable -> TaskResult< F, A... > {
				return std::invoke(
					std::move( fn ),
					std::move( values )...
				);
			};

			auto [ result, job ] = CreateTask( std::move( callable ) );

			EnqueueJob( std::move( job ) );

			return std::move( result );
		}

		// Callable must return sl::Awaitable<R>. It is first invoked on a worker.
		template < typename F, typename... A >
		auto CoSubmit( F&& function, A&&... args )
			-> Awaitable< typename thread::AwaitableValue< TaskResult< F, A... > >::type >
		{
			using R = typename thread::AwaitableValue< TaskResult< F, A... > >::type;
			{
				std::scoped_lock lock( mQueueMutex );
				if ( !mRunning )
					throw std::runtime_error( "Cannot submit to a stopped ThreadPool" );
				++mActiveCoroutines;
			}
			try
			{
				return RunCoroutine< R >( this, std::decay_t< F >( std::forward< F >( function ) ), std::decay_t< A >( std::forward< A >( args ) )... );
			}
			catch ( ... )
			{
				FinishCoroutine();
				throw;
			}
		}

		// Always yields and next executes on a pool worker.
		struct SwitchAwaiter
		{
			ThreadPool* pool;
			bool await_ready() const noexcept
			{
				return false;
			}
			void await_suspend( std::coroutine_handle<> handle ) const
			{
				pool->ScheduleCoroutine( handle );
			}
			void await_resume() const noexcept
			{}
		};
		[[nodiscard]] SwitchAwaiter Schedule() noexcept
		{
			return { this };
		}

		// start(callback) must initiate NON-BLOCKING I/O and invoke callback once.
		// The callback receives Result<T, std::exception_ptr>.
		// It may be invoked on any thread; coroutine resumption is queued on this pool.
		template < typename T, typename Start >
		class IOAwaiter
		{
		public:
			using Result = Result< T, std::exception_ptr >;
			using Callback = std::function< void( Result ) >;

			IOAwaiter( ThreadPool* pool, Start start )
				: pool_( pool ), start_( std::move( start ) ), result_( std::make_shared< Storage >() )
			{}
			bool await_ready() const noexcept
			{
				return false;
			}
			void await_suspend( std::coroutine_handle<> handle )
			{
				auto state = result_;
				auto pool = pool_;

				Callback completed = [ state, pool, handle ]( Result result ) mutable {
					{
						std::scoped_lock lock( state->mutex );
						if ( state->result )
							std::terminate(); // Callback contract: exactly once.
						state->result = std::move( result );
					}
					pool->ScheduleCoroutine( handle );
				};

				try
				{
					std::invoke( std::move( start_ ), completed );
				}
				catch ( ... )
				{
					completed( std::unexpected( std::current_exception() ) );
				}
			}
			decltype( auto ) await_resume()
			{
				auto& result = *result_->result;
				if ( !result )
					std::rethrow_exception( result.Error() );
				if constexpr ( std::is_void_v< T > )
					return;
				else
					return result.Unwrap();
			}

		private:
			struct Storage
			{
				std::mutex mutex;
				std::optional< Result > result;
			};
			ThreadPool* pool_;
			Start start_;
			std::shared_ptr< Storage > result_;
		};

		template < typename T, typename Start >
		auto AwaitIO( Start&& start )
		{
			return IOAwaiter< T, std::decay_t< Start > >( this, std::forward< Start >( start ) );
		}

		template < typename F, typename... A >
		void Dispatch( F&& function, A&&... args )
		{
			EnqueueJob(
				[ fn = std::forward< F >( function ),
				  ... values = std::forward< A >( args ) ] mutable {
					std::invoke(
						std::move( fn ),
						std::move( values )...
					);
				}
			);
		}

	private:
		template < typename >
		friend class Task;

		using QueueJob = std::move_only_function< void() >;

		template < typename Callable >
		auto CreateTask( Callable&& callable )
		{
			using R = std::invoke_result_t<
				std::decay_t< Callable > >;

			std::packaged_task< R() > packaged(
				std::forward< Callable >( callable )
			);

			auto state = std::make_shared< thread::State< R > >( packaged.get_future() );

			QueueJob job = [ packaged = std::move( packaged ),
							 state ] mutable {
				packaged();
				state->Complete();
			};

			return std::pair{
				Task< R >( this, std::move( state ) ),
				std::move( job )
			};
		}

		template < typename R, typename F, typename... A >
		static Awaitable< R > RunCoroutine( ThreadPool* pool, F fn, A... args )
		{
			struct Guard
			{
				ThreadPool* pool;
				~Guard()
				{
					pool->FinishCoroutine();
				}
			} guard{ pool };
			co_await pool->Schedule();
			if constexpr ( std::is_void_v< R > )
			{
				co_await std::invoke( std::move( fn ), std::move( args )... );
				co_return;
			}
			else
				co_return co_await std::invoke( std::move( fn ), std::move( args )... );
		}
		void FinishCoroutine();
		void ScheduleCoroutine( std::coroutine_handle<> handle );

		void EnqueueJob(
			QueueJob job,
			bool continuation = false
		);

		void StartWorkerLoops( int num_threads );
		void StopWorkerLoops();
		void RunWorkerLoop();

		static std::size_t GetInitialThreadCount(
			int num_threads
		);

	private:
		thread::CoroutineContext mCoroutineContext;
		bool mRunning = false;

		mutable std::mutex mLifecycleMutex;
		std::vector< std::thread > mThreads;

		std::mutex mQueueMutex;
		std::condition_variable mQueueCV;

		std::size_t mOutstanding = 0;
		std::size_t mActiveCoroutines = 0;
		std::queue< QueueJob > mJobQueue;
	};

	// Context-aware I/O: no pool argument in coroutine functions.
	template < typename T, typename Start >
	class ContextIOAwaiter
	{
	public:
		using Result = Result< T, std::exception_ptr >;
		using Callback = std::function< void( Result ) >;
		explicit ContextIOAwaiter( Start start ) : start_( std::move( start ) ), result_( std::make_shared< Storage >() )
		{}
		bool await_ready() const noexcept
		{
			return false;
		}
		void await_suspend( std::coroutine_handle<> handle )
		{
			const auto* context = &thread::CurrentContext();
			auto state = result_;
			Callback complete = [ state, context, handle ]( Result result ) mutable {
				{
					std::scoped_lock lock( state->mutex );
					if ( state->result )
						std::terminate();
					state->result = std::move( result );
				}
				context->schedule( handle );
			};
			try
			{
				std::invoke( std::move( start_ ), complete );
			}
			catch ( ... )
			{
				complete( Err<Result>( std::current_exception() ) );
			}
		}
		decltype( auto ) await_resume()
		{
			auto& result = *result_->result;
			if ( !result )
				std::rethrow_exception( result.Error() );
			if constexpr ( std::is_void_v< T > )
				return;
			else
				return result.Unwrap();
		}

	private:
		struct Storage
		{
			std::mutex mutex;
			std::optional< Result > result;
		};
		Start start_;
		std::shared_ptr< Storage > result_;
	};

	template < typename T, typename Start >
	auto AwaitIO( Start&& start )
	{
		return ContextIOAwaiter< T, std::decay_t< Start > >( std::forward< Start >( start ) );
	}

	// Task<T>::Then() requires the complete ThreadPool definition.
	template < typename T >
	template < typename F >
	auto Task< T >::Then( F&& fn ) && -> Task< thread::ContinuationResultT< T, F > >
	{
		if ( !state_ )
			throw std::future_error( std::future_errc::no_state );

		using R = thread::ContinuationResultT< T, F >;

		auto previous = std::move( state_->future );

		auto callable = [ previous = std::move( previous ), fn = std::forward< F >( fn ) ] mutable -> R {
			if constexpr ( std::is_void_v< T > )
			{
				previous.get();
				return std::invoke( std::move( fn ) );
			}
			else
			{
				return std::invoke(
					std::move( fn ),
					previous.get()
				);
			}
		};

		auto [ next, job ] = pool_->CreateTask( std::move( callable ) );

		thread::Continuation callback = [ pool = pool_,
										  job = std::move( job ) ] mutable {
			pool->EnqueueJob( std::move( job ), true );
		};

		if ( !state_->Register( callback ) )
			callback();

		state_.reset();

		return next;
	}

} // namespace sl
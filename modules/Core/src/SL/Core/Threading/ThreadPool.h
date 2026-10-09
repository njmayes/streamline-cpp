#pragma once

#include "Task.h"

namespace sl {

	class ThreadPool
	{
	public:
		struct Args
		{
			// When <= 0, uses max(1, hardware_concurrency() - |num_threads|).
			int num_threads{};
			bool autostart{ true };
		};

	public:
		template < typename Function, typename... Arguments >
		using TaskResult = std::invoke_result_t< std::decay_t< Function >, std::decay_t< Arguments >... >;

		template < typename Function, typename... Arguments >
		using TaskType = Task< TaskResult< Function, Arguments... > >;

	private:
		using QueueJob = std::move_only_function< void() >;

		using ThreadPoolStorage = std::vector< std::thread >;
		using TaskQueueStorage = std::queue< QueueJob >;

	public:
		ThreadPool();
		explicit ThreadPool( Args const& args );
		~ThreadPool();

		ThreadPool( ThreadPool const& ) = delete;
		ThreadPool( ThreadPool&& ) = delete;
		ThreadPool& operator=( ThreadPool const& ) = delete;
		ThreadPool& operator=( ThreadPool&& ) = delete;

		void Start( int num_threads = 0 );

		// Drains queued jobs and their previously registered continuations.
		// Not callable from a pool worker. Throws if lifecycle lock is busy.
		void Stop();
		auto GetThreadCount() const -> std::size_t;

		template < typename Function, typename... Arguments >
		auto Submit( Function&& function, Arguments&&... args ) -> TaskType< Function, Arguments... >
		{
			auto callable = [ function = std::forward< Function >( function ),
							  ... args = std::forward< Arguments >( args ) ] mutable -> TaskResult< Function, Arguments... > {
				return std::invoke( std::move( function ), std::move( args )... );
			};
			auto [ result, job ] = CreateScheduledTask( std::move( callable ) );
			EnqueueJob( std::move( job ) );
			return std::move( result );
		}

		template < typename Function, typename... Arguments >
		void Dispatch( Function&& function, Arguments&&... args )
		{
			EnqueueJob( [ function = std::forward< Function >( function ),
						  ... args = std::forward< Arguments >( args ) ] mutable {
				std::invoke( std::move( function ), std::move( args )... );
			} );
		}

	private:
		template < typename Callable >
		auto CreateScheduledTask( Callable&& callable ) -> std::pair< TaskType< Callable >, QueueJob >
		{
			using Result = TaskResult< Callable >;
			using PackagedTask = std::packaged_task< Result() >;
			using TaskState = Task< Result >::State;

			PackagedTask packaged( std::forward< Callable >( callable ) );
			auto state = std::make_shared< TaskState >( packaged.get_future() );
			QueueJob job = [ packaged = std::move( packaged ), state ] mutable {
				packaged();
				state->Complete();
			};
			return std::pair{ Task< Result >( this, std::move( state ) ), std::move( job ) };
		}

		void StartWorkerLoops( int num_threads );
		void StopWorkerLoops();
		void RunWorkerLoop();

		void EnqueueJob( QueueJob task, bool continuation = false );

		static std::size_t GetInitialThreadCount( int num_threads );

	private:
		bool mRunning{};

		mutable std::mutex mLifecycleMutex;
		ThreadPoolStorage mThreads;

		std::mutex mQueueMutex;
		std::condition_variable mQueueCV;

		TaskQueueStorage mJobQueue;
	};
} // namespace sl
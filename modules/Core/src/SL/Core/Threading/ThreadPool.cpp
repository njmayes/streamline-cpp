#include "ThreadPool.h"

#include <SL/Core/Logging/Log.h>

namespace sl {

	ThreadPool::ThreadPool()
		: ThreadPool( Args{} )
	{}

	ThreadPool::ThreadPool( const Args& args )
	{
		mCoroutineContext.schedule = [ this ]( std::coroutine_handle<> handle ) { ScheduleCoroutine( handle ); };
		if ( args.autostart )
			Start( args.num_threads );
	}

	ThreadPool::~ThreadPool()
	{
		Stop();
	}

	void ThreadPool::Start( int n )
	{
		std::scoped_lock lifecycle( mLifecycleMutex );

		{
			std::scoped_lock queue( mQueueMutex );

			if ( mRunning )
				throw std::logic_error(
					"ThreadPool is already running"
				);
		}

		StartWorkerLoops( n );
	}

	void ThreadPool::Stop()
	{
		std::unique_lock lifecycle(
			mLifecycleMutex,
			std::try_to_lock
		);

		if ( !lifecycle.owns_lock() )
			throw std::logic_error(
				"ThreadPool lifecycle operation already in progress"
			);

		if ( std::ranges::any_of(
				 mThreads,
				 []( const auto& thread ) {
					 return thread.get_id() ==
							std::this_thread::get_id();
				 }
			 ) )
		{
			throw std::logic_error(
				"Cannot stop ThreadPool from a worker thread"
			);
		}

		StopWorkerLoops();
	}

	std::size_t ThreadPool::GetThreadCount() const
	{
		std::scoped_lock lifecycle( mLifecycleMutex );
		return mThreads.size();
	}

	std::size_t ThreadPool::GetInitialThreadCount( int count )
	{
		const auto hardware =
			static_cast< std::int64_t >(
				std::thread::hardware_concurrency()
			);

		const auto requested =
			count <= 0
				? hardware + static_cast< std::int64_t >( count )
				: static_cast< std::int64_t >( count );

		return static_cast< std::size_t >(
			std::max< std::int64_t >( 1, requested )
		);
	}

	void ThreadPool::StartWorkerLoops( int n )
	{
		const auto count = GetInitialThreadCount( n );

		mThreads.reserve( count );

		{
			std::scoped_lock queue( mQueueMutex );
			mRunning = true;
		}

		try
		{
			for ( std::size_t i = 0; i < count; ++i )
			{
				mThreads.emplace_back(
					&ThreadPool::RunWorkerLoop,
					this
				);
			}
		}
		catch ( ... )
		{
			StopWorkerLoops();
			throw;
		}
	}

	void ThreadPool::StopWorkerLoops()
	{
		{
			std::scoped_lock queue( mQueueMutex );
			mRunning = false;
		}

		mQueueCV.notify_all();

		for ( auto& thread : mThreads )
		{
			if ( thread.joinable() )
				thread.join();
		}

		mThreads.clear();
	}

	void ThreadPool::EnqueueJob(
		QueueJob job,
		bool continuation
	)
	{
		{
			std::scoped_lock queue( mQueueMutex );

			if ( !mRunning && !continuation )
			{
				throw std::runtime_error(
					"Cannot queue work on a stopped ThreadPool"
				);
			}

			mJobQueue.emplace( std::move( job ) );
			++mOutstanding;
		}

		mQueueCV.notify_one();
	}

	void ThreadPool::ScheduleCoroutine( std::coroutine_handle<> handle )
	{
		EnqueueJob( [ this, handle ] { thread::Resume( handle, &mCoroutineContext ); }, true );
	}

	void ThreadPool::FinishCoroutine()
	{
		{
			std::scoped_lock lock( mQueueMutex );
			--mActiveCoroutines;
		}
		mQueueCV.notify_all();
	}

	void ThreadPool::RunWorkerLoop()
	{
		while ( true )
		{
			QueueJob job;

			{
				std::unique_lock lock( mQueueMutex );

				mQueueCV.wait(
					lock,
					[ this ] {
						return !mJobQueue.empty() ||
							   ( !mRunning && mOutstanding == 0 && mActiveCoroutines == 0 );
					}
				);

				if ( !mRunning && mOutstanding == 0 && mActiveCoroutines == 0 )
					return;

				job = std::move( mJobQueue.front() );
				mJobQueue.pop();
			}

			try
			{
				job();
			}
			catch ( const std::exception& e )
			{
				log::Error(
					"Exception occurred inside ThreadPool job:"
				);
				log::Error( e.what() );
			}
			catch ( ... )
			{
				log::Error(
					"Unknown exception occurred inside ThreadPool job"
				);
			}

			{
				std::scoped_lock lock( mQueueMutex );
				--mOutstanding;
			}

			mQueueCV.notify_all();
		}
	}

} // namespace sl
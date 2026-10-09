#include "ThreadPool.h"

#include <SL/Core/Logging/Log.h>

namespace sl {

	ThreadPool::ThreadPool() : ThreadPool( Args{} )
	{}

	ThreadPool::ThreadPool( Args const& args )
	{
		if ( args.autostart )
			Start( args.num_threads );
	}

	ThreadPool::~ThreadPool()
	{
		Stop();
	}

	void ThreadPool::Start( int num_threads )
	{
		std::scoped_lock lifecycle_lock( mLifecycleMutex );
		{
			std::scoped_lock queue_lock( mQueueMutex );
			if ( mRunning )
				throw std::logic_error( "ThreadPool is already running" );
		}
		StartWorkerLoops( num_threads );
	}

	void ThreadPool::Stop()
	{
		std::unique_lock lock( mLifecycleMutex, std::try_to_lock );
		if ( !lock.owns_lock() )
			throw std::logic_error( "ThreadPool lifecycle operation already in progress" );

		if ( std::ranges::any_of( mThreads, []( std::thread const& thread ) {
				 return thread.get_id() == std::this_thread::get_id();
			 } ) )
			throw std::logic_error( "Cannot stop ThreadPool from a worker thread" );

		StopWorkerLoops();
	}

	auto ThreadPool::GetThreadCount() const -> std::size_t
	{
		std::scoped_lock lock( mLifecycleMutex );
		return mThreads.size();
	}

	void ThreadPool::StartWorkerLoops( int num_threads )
	{
		auto count = GetInitialThreadCount( num_threads );
		mThreads.reserve( count );
		{
			std::scoped_lock lock( mQueueMutex );
			mRunning = true;
		}
		try
		{
			for ( std::size_t i = 0; i < count; ++i )
				mThreads.emplace_back( &ThreadPool::RunWorkerLoop, this );
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
			std::scoped_lock lock( mQueueMutex );
			mRunning = false;
		}
		mQueueCV.notify_all();
		for ( auto& thread : mThreads )
			if ( thread.joinable() )
				thread.join();
		mThreads.clear();
	}

	void ThreadPool::RunWorkerLoop()
	{
		while ( true )
		{
			QueueJob job;
			{
				std::unique_lock lock( mQueueMutex );
				mQueueCV.wait( lock, [ this ] { return !mRunning || !mJobQueue.empty(); } );
				if ( !mRunning && mJobQueue.empty() )
					return;
				job = std::move( mJobQueue.front() );
				mJobQueue.pop();
			}
			try
			{
				job();
			}
			catch ( std::exception const& e )
			{
				log::Error( "Exception occurred inside ThreadPool job:" );
				log::Error( e.what() );
			}
			catch ( ... )
			{
				log::Error( "Unknown exception occurred inside ThreadPool job" );
			}
		}
	}

	void ThreadPool::EnqueueJob( QueueJob job, bool continuation )
	{
		{
			std::scoped_lock lock( mQueueMutex );
			if ( !mRunning && !continuation )
				throw std::runtime_error( "Cannot queue work on a stopped ThreadPool" );
			mJobQueue.emplace( std::move( job ) );
		}
		mQueueCV.notify_one();
	}

	std::size_t ThreadPool::GetInitialThreadCount( int num_threads )
	{
		auto const hardware = static_cast< std::int64_t >( std::thread::hardware_concurrency() );
		auto const requested = num_threads <= 0 ? hardware + static_cast< std::int64_t >( num_threads )
												: static_cast< std::int64_t >( num_threads );
		return static_cast< std::size_t >( std::max< std::int64_t >( 1, requested ) );
	}

} // namespace sl
#include "ThreadPool.h"

namespace slc {

	ThreadPool::ThreadPool( size_t num_threads )
	{
		auto thread_queuer = [ this ] {
			while ( true )
			{
				std::move_only_function< void() > task;

				{
					std::unique_lock< std::mutex > lock( mQueueMutex );

					// Wait until there is a task to
					// execute or the pool is stopped
					mCV.wait( lock, [ this ] { return !mTasks.empty() || mStop; } );

					if ( mStop && mTasks.empty() )
						return;

					// Get the next task from the queue
					task = std::move( mTasks.front() );
					mTasks.pop();
				}

				task();
			}
		};

		for ( size_t i = 0; i < num_threads; ++i )
		{
			mThreads.emplace_back( thread_queuer );
		}
	}

	ThreadPool::~ThreadPool()
	{
		Shutdown();
	}

	void ThreadPool::Shutdown()
	{
		{
			std::unique_lock< std::mutex > lock( mQueueMutex );
			mStop = true;
		}

		mCV.notify_all();

		for ( auto& thread : mThreads )
			thread.join();
	}
} // namespace slc
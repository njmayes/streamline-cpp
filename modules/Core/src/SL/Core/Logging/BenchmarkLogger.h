
#pragma once

#include "Logger.h"

namespace sl {

	struct BenchmarkLogger
	{
		Logger& logger;

		explicit BenchmarkLogger( Logger& logger )
			: logger{ logger }
		{
			BenchmarkStopWorker();
		}

		void BenchmarkFlush()
		{
			SL_ASSERT( not logger.mWorker.joinable(), "Stop the logger worker before benchmarking" );

			// Flush pending buffers in FIFO order.
			while ( true )
			{
				std::optional< std::size_t > index;

				{
					std::scoped_lock lock( logger.mQueueMutex );

					if ( logger.mPendingBuffers.Empty() )
						break;

					index = logger.mPendingBuffers.Pop();
				}

				auto& storage = logger.GetStorage( *index );

				logger.Flush( storage );
				storage.Reset();

				{
					std::scoped_lock lock( logger.mQueueMutex );

					logger.mStats.Flush( logger.mLastFlush );
					logger.mFreeBuffers.Push( *index );
				}
			}

			// Flush the currently active buffer, if any.
			{
				std::scoped_lock lock( logger.mQueueMutex );

				logger.ActivateFreeStorage();
			}

			Logger::MessageStorage* storage{};

			{
				std::scoped_lock lock( logger.mQueueMutex );

				if ( logger.mActiveBuffer.has_value() )
				{
					auto& active = logger.GetActiveStorage();

					if ( not active.queue.empty() )
						storage = &active;
				}
			}

			if ( storage )
			{
				logger.Flush( *storage );
				storage->Reset();

				std::scoped_lock lock( logger.mQueueMutex );
				logger.mStats.Flush( logger.mLastFlush );
			}
		}

		void BenchmarkStopWorker()
		{
			{
				std::scoped_lock lock( logger.mQueueMutex );
				logger.mTerminate = true;
			}

			logger.mWorkerCV.notify_all();
			logger.mProducerCV.notify_all();

			if ( logger.mWorker.joinable() )
				logger.mWorker.join();

			// Keep mTerminate true so normal logging cannot
			// submit messages without a running worker.
		}

		void BenchmarkResetState()
		{
			SL_ASSERT( not logger.mWorker.joinable(), "Stop the logger worker before resetting" );

			std::scoped_lock lock( logger.mQueueMutex );

			for ( auto& storage : logger.mMessageStorage )
				storage->Reset();

			while ( not logger.mPendingBuffers.Empty() )
				logger.mPendingBuffers.Pop();

			while ( not logger.mFreeBuffers.Empty() )
				logger.mFreeBuffers.Pop();

			logger.mActiveBuffer.reset();

			for ( std::size_t i = 0; i < logger.mMessageStorage.size(); ++i )
				logger.mFreeBuffers.Push( i );

			logger.ActivateFreeStorage();

			logger.mStats.Reset();
			logger.mLastFlush = Clock::now();
		}

		auto& BenchmarkQueue()
		{
			SL_ASSERT( not logger.mWorker.joinable(), "Stop the logger worker before accessing the queue" );

			std::scoped_lock lock( logger.mQueueMutex );

			logger.ActivateFreeStorage();

			return logger.GetActiveStorage().queue;
		}

		auto& BenchmarkArena()
		{
			SL_ASSERT( not logger.mWorker.joinable(), "Stop the logger worker before accessing the arena" );

			std::scoped_lock lock( logger.mQueueMutex );

			logger.ActivateFreeStorage();

			return logger.GetActiveStorage().arena;
		}

		auto& BenchmarkTargets()
		{
			return logger.mLogTargets;
		}

		std::size_t MessageSizeLimit() const
		{
			return logger.mBufferSizeLimit - 1;
		}
	};
} // namespace sl

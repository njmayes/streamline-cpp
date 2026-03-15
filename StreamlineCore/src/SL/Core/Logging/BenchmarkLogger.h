#pragma once

#include "Logger.h"

namespace sl {

	struct BenchmarkLogger
	{
		Logger& logger;

		BenchmarkLogger( Logger& logger )
			: logger{ logger }
		{
			{
				std::unique_lock< std::mutex > lock( logger.mQueueMutex );
				logger.mTerminate = true;
			}

			logger.mCV.notify_all();

			if ( logger.mWorker.joinable() )
				logger.mWorker.join();

			logger.mTerminate = false;

			// Keep destructor logic safe.
			logger.mWorker = std::thread( [] {} );
		}

		void BenchmarkFlush()
		{
			logger.Flush();
		}

		void BenchmarkStopWorker()
		{
			{
				std::unique_lock< std::mutex > lock( logger.mQueueMutex );
				logger.mTerminate = true;
			}

			logger.mCV.notify_all();

			if ( logger.mWorker.joinable() )
				logger.mWorker.join();

			logger.mTerminate = false;

			// Keep destructor logic safe.
			logger.mWorker = std::thread( [] {} );
		}

		void BenchmarkResetState()
		{
			logger.mMessageQueue.clear();
			logger.mArena.ReleaseBuffers();
			logger.mStats.messages_since_last_flush = 0;
			logger.mStats.large_message_count = 0;
		}

		auto& BenchmarkQueue()
		{
			return logger.mMessageQueue;
		}

		auto& BenchmarkArena()
		{
			return logger.mArena;
		}

		auto& BenchmarkTargets()
		{
			return logger.mLogTargets;
		}

		std::size_t MessageSizeLimit() const
		{
			return logger.mMessageSizeLimit;
		}
	};
} // namespace sl
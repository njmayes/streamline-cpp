
#include "Logger.h"

#include "BenchmarkLogger.h"

#include "Targets/FileLogTarget.h"

#include "SL/Core/Common/Time.h"
#include "SL/Core/Types/ScopedTimer.h"

namespace {

	static sl::Logger& MakeErrorLogger()
	{
		static sl::Logger logger;
		logger.AddLogTarget< sl::FileLogTarget >( "log_error.log", sl::LogLevel::Error );
		return logger;
	}
} // namespace

namespace sl {

	Logger& Logger::GetGlobalLogger()
	{
		static Logger sLogger;
		return sLogger;
	}

	Logger& Logger::GetErrorLogger()
	{
		static Logger& sLogger = MakeErrorLogger();
		return sLogger;
	}

	Logger::Logger(
		std::size_t buffer_size_limit,
		std::size_t max_messages_before_flush,
		std::size_t buffer_count
	)
		: mBufferSizeLimit( buffer_size_limit )
		, mMaxMessagesBeforeFlush( max_messages_before_flush )
		, mFreeBuffers( buffer_count )
		, mPendingBuffers( buffer_count )
	{
		if ( buffer_count < 2 )
			throw std::invalid_argument( "Logger requires at least two buffers" );

		if ( buffer_size_limit < 2 or max_messages_before_flush == 0 )
			throw std::invalid_argument( "Invalid logger buffer configuration" );

		if ( buffer_count > std::numeric_limits< std::size_t >::max() / buffer_size_limit )
			throw std::length_error( "Logger buffer configuration overflow" );

		mMessageStorage.reserve( buffer_count );

		for ( std::size_t i = 0; i < buffer_count; ++i )
		{
			mMessageStorage.emplace_back(
				std::make_unique< MessageStorage >(
					buffer_size_limit,
					max_messages_before_flush
				)
			);

			mFreeBuffers.Push( i );
		}

		ActivateFreeStorage();

		mWorker = std::thread( &Logger::ProcessQueue, this );
	}

	Logger::~Logger()
	{
		{
			std::scoped_lock lock( mQueueMutex );
			mTerminate = true;
		}

		mWorkerCV.notify_all();
		mProducerCV.notify_all();

		if ( mWorker.joinable() )
			mWorker.join();
	}

	void Logger::SetLogLevel( LogLevel level )
	{
		mMinLogLevel.store( level, std::memory_order_relaxed );
	}

	void Logger::Log( LogLevel level, std::string_view message )
	{
		SL_PROFILE_FUNCTION();

		if ( std::to_underlying( level ) <
			 std::to_underlying( mMinLogLevel.load( std::memory_order_relaxed ) ) )
			return;

		auto level_string = Enum::ToString( level );
		auto& timestamp = GetCurrentTimestamp();

		bool notify_worker = false;

		{
			std::unique_lock lock( mQueueMutex );

			{
				SL_PROFILE_SCOPE( "Logging - Wait for storage" );

				mProducerCV.wait( lock, [ this ] {
					return mTerminate or mActiveBuffer.has_value() or not mFreeBuffers.Empty();
				} );
			}

			if ( mTerminate )
				return;

			// Another producer might have filled the previous active
			// buffer. Claim a free one if required.
			ActivateFreeStorage();

			auto& storage = GetActiveStorage();

			auto buffer = storage.arena.RequestBuffer( mBufferSizeLimit );

			if ( not buffer.has_value() )
				throw std::runtime_error( "Logger arena exhausted unexpectedly" );

			std::size_t formatted_size{};

			{
				SL_PROFILE_SCOPE( "Logging - Format message" );

				auto result = std::format_to_n(
					buffer->begin(),
					mBufferSizeLimit - 1,
					"[{}] {}: {}",
					level_string,
					timestamp.data(),
					message
				);

				formatted_size = std::min(
					static_cast< std::size_t >( result.size ),
					mBufferSizeLimit - 1
				);

				*( buffer->begin() + formatted_size ) = '\0';
			}

			{
				SL_PROFILE_SCOPE( "Logging - Push message" );

				storage.queue.emplace_back(
					*buffer,
					formatted_size + 1,
					level
				);
			}

			mStats.AddMessage( formatted_size, mBufferSizeLimit - 1 );

			// Immediately rotate to the next free buffer when full.
			// This does not wait for the worker to finish flushing.
			if ( storage.queue.size() >= mMaxMessagesBeforeFlush )
			{
				QueueActiveStorage();
				ActivateFreeStorage();

				notify_worker = true;
			}
		}

		if ( notify_worker )
		{
			mWorkerCV.notify_one();

			// Other producers may be waiting for an active buffer.
			mProducerCV.notify_all();
		}
	}

	void Logger::ProcessQueue()
	{
		SL_PROFILE_FUNCTION();

		while ( true )
		{
			std::optional< std::size_t > flush_index;
			bool notify_producers = false;

			{
				std::unique_lock lock( mQueueMutex );

				{
					SL_PROFILE_SCOPE( "Logging - Wait for flush" );

					mWorkerCV.wait_until(
						lock,
						mLastFlush + MaxTimeBetweenFlush,
						[ this ] {
							return mTerminate or not mPendingBuffers.Empty();
						}
					);
				}

				// On shutdown, enqueue the remaining partial active
				// buffer after all previously completed buffers.
				if ( mTerminate )
				{
					if ( mActiveBuffer.has_value() )
						QueueActiveStorage();
				}
				else if ( mPendingBuffers.Empty() )
				{
					// Reached the time-based flush deadline.
					// A partially filled active buffer must also
					// be submitted for flushing.
					if ( mActiveBuffer.has_value() )
					{
						auto& active = GetActiveStorage();

						if ( not active.queue.empty() )
						{
							QueueActiveStorage();
							ActivateFreeStorage();

							notify_producers = true;
						}
					}
				}

				if ( mPendingBuffers.Empty() )
				{
					if ( mTerminate )
						break;

					// Avoid repeatedly waiting on an expired deadline
					// while no messages are available.
					mLastFlush = Clock::now();
					continue;
				}

				flush_index = mPendingBuffers.Pop();

				// If producers have no active buffer but a free one
				// exists, activate it before releasing the mutex.
				if ( not mTerminate and not mActiveBuffer.has_value() )
				{
					ActivateFreeStorage();
					notify_producers = true;
				}
			}

			if ( notify_producers )
				mProducerCV.notify_all();

			auto& storage = GetStorage( *flush_index );

			// Flush outside the queue mutex.
			// No producer can access this storage while it is owned
			// by the worker.
			try
			{
				Flush( storage );
			}
			catch ( std::exception const& exception )
			{
				std::fprintf(
					stderr,
					"Logger target flush failed: %s\n",
					exception.what()
				);
			}
			catch ( ... )
			{
				std::fputs(
					"Logger target flush failed with an unknown exception\n",
					stderr
				);
			}

			// The worker exclusively owns this storage until it
			// returns the index to the free pool.
			storage.Reset();

			{
				std::scoped_lock lock( mQueueMutex );

				mStats.Flush( mLastFlush );

				mFreeBuffers.Push( *flush_index );

				if ( not mTerminate )
					ActivateFreeStorage();
			}

			mProducerCV.notify_all();
		}
	}

	void Logger::Flush( MessageStorage& storage )
	{
		SL_PROFILE_FUNCTION();

		{
			SL_PROFILE_SCOPE( "Logging - Write targets" );

			std::scoped_lock lock( mTargetsMutex );

			for ( auto const& target : mLogTargets )
			{
				target->PreFlush();
				target->WriteTarget( storage.queue );
				target->Flush();
			}
		}
	}

	Logger::MessageStorage& Logger::GetStorage( std::size_t index )
	{
		return *mMessageStorage[ index ];
	}

	Logger::MessageStorage& Logger::GetActiveStorage()
	{
		SL_ASSERT( mActiveBuffer.has_value(), "No active logger buffer" );
		return GetStorage( *mActiveBuffer );
	}

	void Logger::ActivateFreeStorage()
	{
		// Must be called while holding mQueueMutex, or during
		// construction before the worker starts.
		if ( mActiveBuffer.has_value() or mFreeBuffers.Empty() )
			return;

		mActiveBuffer = mFreeBuffers.Pop();

		SL_ASSERT(
			GetActiveStorage().queue.empty(),
			"Activated logger buffer is not empty"
		);
	}

	void Logger::QueueActiveStorage()
	{
		// Caller must hold mQueueMutex.
		if ( not mActiveBuffer.has_value() )
			return;

		auto const index = *mActiveBuffer;

		if ( not GetStorage( index ).queue.empty() )
		{
			mPendingBuffers.Push( index );
		}
		else
		{
			mFreeBuffers.Push( index );
		}

		mActiveBuffer.reset();
	}

	Logger::TemporaryBuffer& Logger::GetCurrentTimestamp()
	{
		SL_PROFILE_FUNCTION();

		thread_local TimestampCache tsCache;

		auto now = std::chrono::system_clock::now();

		if ( std::chrono::floor< std::chrono::seconds >( now ) ==
			 std::chrono::floor< std::chrono::seconds >( tsCache.timestamp ) )
			return tsCache.format_string;

		auto now_c = std::chrono::system_clock::to_time_t( now );
		auto time = GetLocalTime( &now_c );

		std::strftime(
			tsCache.format_string.data(),
			tsCache.format_string.size(),
			"%F %T",
			&time
		);

		tsCache.timestamp = now;

		return tsCache.format_string;
	}

	BenchmarkLogger Logger::GetBenchmarkLogger()
	{
		return BenchmarkLogger( *this );
	}
} // namespace sl

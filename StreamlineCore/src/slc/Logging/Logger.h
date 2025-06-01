#pragma once

#include "LogMemoryArena.h"
#include "Targets/ILogTarget.h"

#include "slc/Common/Profiling.h"

#include <thread>
#include <mutex>

namespace slc {

	class Logger
	{
	private:
		SCONSTEXPR std::size_t MessageSizeLimit = 512;
		SCONSTEXPR std::size_t MaxMessagesBeforeFlush = 1024;
		SCONSTEXPR std::size_t TemporaryBufferSize = MessageSizeLimit;

		using TemporaryBuffer = std::array< char, MessageSizeLimit >;

		using Clock = std::chrono::steady_clock;
		using Duration = std::chrono::duration< float, std::micro >;
		using TimePoint = std::chrono::time_point< Clock, Duration >;

		SCONSTEXPR Duration MaxTimeBetweenFlush = Duration{ std::chrono::milliseconds( 100 ) };

		struct LoggerStats
		{
			std::size_t total_flushes{};
			std::size_t messages_since_last_flush{};
			Duration time_since_last_flush{};
			std::size_t large_message_count{};
		};

	public:
		static Logger& GetGlobalLogger();
		static Logger& GetErrorLogger();

		Logger( std::size_t message_size_limit = MessageSizeLimit, std::size_t max_messages_before_flus = MaxMessagesBeforeFlush );
		~Logger();

		LoggerStats const& GetStats() const
		{
			return mStats;
		}

		void SetLogLevel( LogLevel level );

		template < typename target_t, typename... Args >
			requires std::derived_from< target_t, ILogTarget > and
					 std::constructible_from< target_t, Args... >
		void AddLogTarget( Args&&... args )
		{
			auto target = MakeUnique< target_t >( std::forward< Args >( args )... );
			mLogTargets.push_back( std::move( target ) );
		}

		void Log( LogLevel level, std::string_view message );

		template < typename... Args >
		void Log( LogLevel level, std::format_string< Args... > message, Args&&... args )
		{
			SLC_PROFILE_FUNCTION();

			if ( std::to_underlying( level ) < std::to_underlying( mMinLogLevel ) )
				return;

			auto buffer = mArena.RequestBuffer( mMessageSizeLimit );
			if ( not buffer.has_value() )
			{
				{
					SLC_PROFILE_SCOPE( "Logging - Notify" );
					mCV.notify_one();
				}

				{
					SLC_PROFILE_SCOPE( "Logging - Wait" );

					std::unique_lock< std::mutex > lock( mQueueMutex );
					mCV.wait( lock, [ this ] { return mMessageQueue.size() < mMaxMessagesBeforeFlush; } );
				}

				{
					SLC_PROFILE_SCOPE( "Logging - Get Buffer Retry" );

					buffer = mArena.RequestBuffer( mMessageSizeLimit );
					ASSERT( buffer.has_value(), "Still could not create message buffer despite flush" );
				}
			}

			auto level_string = Enum::ToString( level );
			UpdateCurrentTimestamp();

			std::size_t formatted_size{};
			{
				SLC_PROFILE_SCOPE( "Logging - Format message" );

				auto formatted_message = GetFormatMessage( message, std::forward< Args >( args )... );
				auto format_result = std::format_to_n( buffer->begin(), mMessageSizeLimit, "[{}] {}: {}", level_string, mTimestampCache.format_string.data(), formatted_message.data() );
				formatted_size = std::min( static_cast< std::size_t >( format_result.size ), mMessageSizeLimit );
			}


			if ( formatted_size == mMessageSizeLimit )
				mStats.large_message_count++;

			{
				SLC_PROFILE_SCOPE( "Logging - Lock queue and push" );

				std::lock_guard< std::mutex > lock( mQueueMutex );
				mMessageQueue.emplace_back( *buffer, formatted_size, level );
			}

			mStats.messages_since_last_flush++;

			mCV.notify_one();
		}

	private:
		void ProcessQueue();
		void Flush();

		template < typename... Args >
		TemporaryBuffer GetFormatMessage( std::format_string< Args... > message, Args&&... args )
		{
			SLC_PROFILE_FUNCTION();

			TemporaryBuffer temp;
			auto result = std::format_to_n( temp.data(), temp.size(), message, std::forward< Args >( args )... );
			return temp;
		}

		void UpdateCurrentTimestamp();

	private:
		std::thread mWorker;
		std::mutex mQueueMutex;
		std::condition_variable mCV;
		bool mTerminate;

		TimePoint mLastFlush;

		LogLevel mMinLogLevel;
		std::vector< MessageEntry > mMessageQueue;
		std::vector< Unique< ILogTarget > > mLogTargets;

		LogMemoryArena mArena;
		std::vector< char > mMessageBuffer;

		LoggerStats mStats;

		struct TimestampCache
		{
			std::chrono::system_clock::time_point timestamp;
			TemporaryBuffer format_string;
		} mTimestampCache;

		std::size_t mMessageSizeLimit;
		std::size_t mMaxMessagesBeforeFlush;
	};
} // namespace slc
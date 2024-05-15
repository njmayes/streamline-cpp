#include "Logger.h"

#include "Targets/FileLogTarget.h"

#include "slc/Common/Time.h"
#include "slc/Types/ScopedTimer.h"

namespace
{
	static slc::Logger& MakeErrorLogger()
	{
		static slc::Logger logger;
		logger.AddLogTarget<slc::FileLogTarget>("log_error.log", slc::LogLevel::Error);
		return logger;
	}
}


namespace slc {

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

	Logger::Logger(std::size_t message_size_limit, std::size_t max_messages_before_flush)
		: mMinLogLevel(LogLevel::Debug)
		, mTerminate(false)
		, mLastFlush{ Clock::now() }
		, mMessageSizeLimit(message_size_limit)
		, mMaxMessagesBeforeFlush(max_messages_before_flush)
		, mArena(message_size_limit* max_messages_before_flush)
	{
		mWorker = std::thread(&Logger::ProcessQueue, this);
		mMessageBuffer.reserve(mMessageSizeLimit * mMaxMessagesBeforeFlush);
	}

	Logger::~Logger()
	{
		{
			std::unique_lock<std::mutex> lock(mQueueMutex);
			mTerminate = true;
		}
		mCV.notify_all();
		mWorker.join();
	}

	void Logger::SetLogLevel(LogLevel level)
	{
		mMinLogLevel = level;
	}

	void Logger::Log(LogLevel level, std::string_view message)
	{
		SLC_PROFILE_FUNCTION();

		if (std::to_underlying(level) < std::to_underlying(mMinLogLevel))
			return;

		std::optional<MessageBuffer> buffer = mArena.RequestBuffer(mMessageSizeLimit);
		if (not buffer.has_value())
		{
			{
				SLC_PROFILE_SCOPE("Logging - Notify");

				mCV.notify_one();
			}
			{
				SLC_PROFILE_SCOPE("Logging - Wait");

				std::unique_lock<std::mutex> lock(mQueueMutex);
				mCV.wait(lock, [this] { return mMessageQueue.size() < mMaxMessagesBeforeFlush; });
			}

			{
				SLC_PROFILE_SCOPE("Logging - Get Buffer Retry");

				buffer = mArena.RequestBuffer(mMessageSizeLimit);
				ASSERT(buffer.has_value(), "Still could not create message buffer despite flush");
			}
		}

		auto level_string = Enum::ToString(level);

		UpdateCurrentTimestamp();

		std::size_t formatted_size{};
		{
			SLC_PROFILE_SCOPE("Logging - Format message");

			auto format_result = std::format_to_n(buffer->begin(), mMessageSizeLimit, "[{}] {}: {}", level_string, mTimestampCache.format_string.data(), message.data());
			formatted_size = std::min(static_cast<std::size_t>(format_result.size), mMessageSizeLimit);
		}

		if (formatted_size == mMessageSizeLimit)
			mStats.large_message_count++;

		{
			SLC_PROFILE_SCOPE("Logging - Lock queue and push");

			std::lock_guard<std::mutex> lock(mQueueMutex);
			mMessageQueue.emplace_back(*buffer, formatted_size, level);
		}

		mStats.messages_since_last_flush++;

		mCV.notify_one();
	}

	void Logger::ProcessQueue()
	{
		while (true) {
			{
				std::unique_lock<std::mutex> lock(mQueueMutex);
				mCV.wait_until(lock, mLastFlush + MaxTimeBetweenFlush, [this] {
					return mMessageQueue.size() >= mMaxMessagesBeforeFlush or mTerminate;
					});

				Flush();
			}
			mCV.notify_one();

			if (mTerminate and mMessageQueue.empty())
				break;
		}
	}

	void slc::Logger::Flush()
	{
		SLC_PROFILE_FUNCTION();

		{
			SLC_PROFILE_SCOPE("Pre-write");

			for (auto const& target : mLogTargets)
			{
				target->PreFlush();
			}
		}

		{
			SLC_PROFILE_SCOPE("Writing");

			for (auto const& target : mLogTargets)
			{
				target->WriteTarget(mMessageQueue);
				target->Flush();
			}
		}

		{
			SLC_PROFILE_SCOPE("Cleanup");

			{
				SLC_PROFILE_SCOPE("Cleanup - Release buffer");
				mArena.ReleaseBuffers();
			}
			{
				SLC_PROFILE_SCOPE("Cleanup - Clear queue");
				mMessageQueue.clear();
			}
		}

		{
			SLC_PROFILE_SCOPE("Stats");

			mStats.total_flushes++;
			mStats.large_message_count = 0;
			mStats.messages_since_last_flush = 0;

			auto flush_time = Clock::now();
			mStats.time_since_last_flush = flush_time - mLastFlush;
			mLastFlush = flush_time;
		}
	}

	void Logger::UpdateCurrentTimestamp()
	{
		SLC_PROFILE_FUNCTION();

		std::chrono::system_clock::time_point now;
		{
			SLC_PROFILE_SCOPE("Update timestamp - Get time");
			now = std::chrono::system_clock::now();
		}

		{
			SLC_PROFILE_SCOPE("Update timestamp - Check if time is same");
			if (std::chrono::floor<std::chrono::seconds>(now) == std::chrono::floor<std::chrono::seconds>(mTimestampCache.timestamp))
				return;
		}

		std::time_t now_c{};
		{
			SLC_PROFILE_SCOPE("Update timestamp - To time_t");
			now_c = std::chrono::system_clock::to_time_t(now);
		}

		std::tm time{};
		{
			SLC_PROFILE_SCOPE("Update timestamp - Get local time");
			time = GetLocalTime(&now_c);
		}

		{
			SLC_PROFILE_SCOPE("Update timestamp - update saved timestamp");
			mTimestampCache.timestamp = now;
		}

		{
			SLC_PROFILE_SCOPE("Update timestamp - Format time");
			std::strftime(mTimestampCache.format_string.data(), mTimestampCache.format_string.size(), "%F %T", &time);
		}
	}
}
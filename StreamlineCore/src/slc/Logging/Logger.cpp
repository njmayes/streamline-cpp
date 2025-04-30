#include "Logger.h"

#include "slc/Common/Time.h"
#include "slc/Types/ScopedTimer.h"

namespace slc {

	Logger& Logger::GetGlobalLogger()
	{
		static Logger sLogger;
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

	void Logger::Log(LogLevel level, std::string_view message)
	{
		if (std::to_underlying(level) < std::to_underlying(mMinLogLevel))
			return;

		auto buffer = mArena.RequestBuffer(mMessageSizeLimit);
		if (not buffer.has_value())
		{
			{
				std::unique_lock<std::mutex> lock(mQueueMutex);
				Flush();
			}

			buffer = mArena.RequestBuffer(mMessageSizeLimit);
			ASSERT(buffer.has_value(), "Still could not create message buffer despite flush");
		}

		std::memset(buffer->data(), 0, buffer->size());

		auto level_string = Enum::ToString(level);
		auto level_format_result = std::format_to_n(buffer->begin(), level_string.size() + 2, "[{}]", level_string.data());

		auto timestamp = GetCurrentTimestamp();
		auto timestamp_format_result = std::format_to_n(buffer->begin(), level_format_result.size + timestamp.used + 3, "{} ({})", buffer->data(), timestamp.buffer.data());

		auto format_result = std::format_to_n(buffer->begin(), mMessageSizeLimit, "{}: {}", buffer->data(), message.data());
		auto formatted_size = std::min(static_cast<std::size_t>(format_result.size), mMessageSizeLimit);

		if (formatted_size == mMessageSizeLimit)
			mStats.large_message_count++;

		{
			std::lock_guard<std::mutex> lock(mQueueMutex);
			mMessageQueue.emplace_back(*buffer, formatted_size, level);
		}

		mStats.messages_since_last_flush++;

		mCV.notify_one();
	}

	void Logger::ProcessQueue()
	{
		while (true) {
			std::unique_lock<std::mutex> lock(mQueueMutex);
			mCV.wait_until(lock, mLastFlush + MaxTimeBetweenFlush, [this] {
				return mMessageQueue.size() >= mMaxMessagesBeforeFlush or mTerminate;
				});

			Flush();

			if (mTerminate and mMessageQueue.empty())
				break;
		}
	}

	void slc::Logger::Flush()
	{
		Timer timer;
		for (auto const& target : mLogTargets)
		{
			target->PreFlush();
		}
		mStats.total_pre_flush_time += Duration{ timer.ElapsedMicros() };

		SLC_TODO("Maybe make concurrent");

		timer.Reset();
		for (auto const& target : mLogTargets)
		{
			target->WriteTarget(mMessageQueue, mMessageBuffer);
			target->Flush();
		}
		mStats.total_write_time += Duration{ timer.ElapsedMicros() };

		for (auto const& entry : mMessageQueue)
			mArena.ReleaseBuffer(entry.message);
		mMessageQueue.clear();

		mStats.total_flushes++;
		mStats.large_message_count = 0;
		mStats.messages_since_last_flush = 0;

		auto flush_time = Clock::now();
		mStats.time_since_last_flush = flush_time - mLastFlush;
		mLastFlush = flush_time;
	}

	Logger::TemporaryBuffer Logger::GetCurrentTimestamp() {
		auto now = std::chrono::system_clock::now();
		auto now_c = std::chrono::system_clock::to_time_t(now);
		auto time = GetLocalTime(&now_c);

		TemporaryBuffer temp{};
		temp.used = std::strftime(temp.buffer.data(), temp.buffer.size(), "%F %T", &time);
		return temp;
	}
}
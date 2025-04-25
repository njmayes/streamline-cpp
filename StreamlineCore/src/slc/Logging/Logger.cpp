#include "Logger.h"

#include "slc/Common/Time.h"

namespace slc {

	Logger::Logger()
		: mMinLogLevel(LogLevel::Debug)
		, mTerminate(false)
		, mLastFlush{ Clock::now() }
	{
		mWorker = std::thread(&Logger::ProcessQueue, this);
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

	void Logger::ProcessQueue()
	{
		while (true) {
			std::unique_lock<std::mutex> lock(mQueueMutex);
			mCV.wait_until(lock, mLastFlush + MaxTimeBetweenFlush, [this] {
				return mMessageQueue.size() >= MaxMessagesBeforeFlush or mTerminate;
				});

			Flush();

			if (mTerminate and mMessageQueue.empty())
				break;
		}
	}

	void slc::Logger::Flush()
	{
		SLC_TODO("Maybe make concurrent");
		for (auto const& message : mMessageQueue)
		{
			for (auto const& target : mLogTargets)
			{
				target->WriteTarget(message);
			}
		}

		for (auto const& target : mLogTargets)
		{
			target->Flush();
		}


		for (auto const& entry : mMessageQueue)
			mArena.ReleaseBuffer(entry.message);
		mMessageQueue.clear();

		mStats.large_message_count = 0;

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

#pragma once

#include "slc/Common/Base.h"
#include "slc/Common/Time.h"

#include <thread>
#include <mutex>
#include <queue>
#include <iostream>

namespace slc
{
	enum class LogLevel {
		Trace,
		Debug,
		Info,
		Warning,
		Error,
		Fatal
	};

	using MessageBuffer = std::span<char>;

	struct MessageEntry
	{
		MessageBuffer message;
		std::size_t length;
		LogLevel level;
	};

	class LogMemoryArena
	{
	public:
		SCONSTEXPR std::size_t DefaultArenaSize = 0x1000000;
		SCONSTEXPR double ReallocScaleFactor = 1.5;

	public:
		LogMemoryArena()
			: mBuffer{ MakeImpl< char[] >(DefaultArenaSize) }
		{

		}

		MessageBuffer RequestBuffer(std::size_t size)
		{
			auto new_tail = mTail + size;
			ASSERT(new_tail > mHead, "Arena ran out of space");

			if (new_tail >= DefaultArenaSize)
			{
				new_tail %= DefaultArenaSize;
				ASSERT(new_tail < mHead, "Arena ran out of space");
			}

			auto buffer = MessageBuffer(mBuffer.get() + mTail, size);
			mTail = new_tail;

			return buffer;
		}

		void ReleaseBuffer(MessageBuffer buffer)
		{
			ASSERT(buffer.data() == (mBuffer.get() + mHead), "Arena is a queue, must release the oldest buffer first");

			auto new_head = mHead + buffer.size();
			ASSERT(new_head <= mTail, "Error, released buffer is larger than currently allocated space");

			mHead = new_head;
		}

	private:
		Impl<char[]> mBuffer;
		std::size_t mHead = 0;
		std::size_t mTail = 0;
	};

	class ILogTarget
	{
	public:
		ILogTarget(LogLevel level)
			: mLogLevel{ level }
		{
		}

		void WriteTarget(std::span<MessageEntry> data)
		{
			for (auto const& entry : data)
			{
				DoWriteTarget(entry);
			}

			DoFlush();
		}

		virtual void DoWriteTarget(MessageEntry const& entry) = 0;
		virtual void DoFlush() = 0;

		void SetLogLevel(LogLevel level) { mLogLevel = level; }
		LogLevel GetLogLevel() const { return mLogLevel; }

	private:
		LogLevel mLogLevel;
	};

	class ConsoleLogTarget : public ILogTarget
	{
	public:
		ConsoleLogTarget(LogLevel level = LogLevel::Debug)
			: ILogTarget(level)
		{
		}

		void DoWriteTarget(MessageEntry const& entry) override
		{
			std::string_view message{ entry.message.data(), entry.length };
			std::cout << message;
		}

		void DoFlush() override
		{
			std::cout << std::flush;
		}
	};


	class FileLogTarget : public ILogTarget
	{
	public:
		FileLogTarget(const std::string& filename, LogLevel level = LogLevel::Debug)
			: ILogTarget(level)
			, mFile(filename, std::ios_base::out | std::ios_base::app)
		{
		}

		void DoWriteTarget(MessageEntry const& entry) override
		{
			std::string_view message{ entry.message.data(), entry.length };
			mFile << message;
		}

		void DoFlush() override
		{
			mFile << std::flush;
		}

	private:
		std::ofstream mFile;
	};



	class Logger
	{
	private:
		SCONSTEXPR std::size_t MessageSizeLimit = 1024;
		SCONSTEXPR std::size_t TemporaryBufferSize = 512;
		SCONSTEXPR std::size_t MaxMessagesBeforeFlush = 64;

		struct TemporaryBuffer
		{
			std::array<char, TemporaryBufferSize> buffer{};
			std::size_t used;
		};

		using Clock = std::chrono::steady_clock;
		using Duration = std::chrono::duration< double >;
		using TimePoint = std::chrono::time_point< Clock, Duration >;

		SCONSTEXPR Duration MaxTimeBetweenFlush = Duration{ std::chrono::milliseconds(100) };

	public:
		Logger()
			: mMinLogLevel(LogLevel::Debug)
			, mTerminate(false)
			, mNextFlush{ Clock::now() + MaxTimeBetweenFlush }
		{
			mWorker = std::thread(&Logger::ProcessQueue, this);
		}

		~Logger()
		{
			{
				std::unique_lock<std::mutex> lock(mQueueMutex);
				mTerminate = true;
			}
			mCV.notify_all();
			mWorker.join();
		}

		template<typename target_t, typename... Args> requires 
			std::derived_from<target_t, ILogTarget> and
			std::constructible_from<target_t, Args...>
		void AddLogTarget(Args&&... args)
		{
			auto target = MakeImpl<target_t>(std::forward<Args>(args)...);
			mLogTargets.push_back(std::move(target));
		}

		template<typename... Args>
		void Log(LogLevel level, std::format_string<Args...> message, Args&&... args)
		{
			if (std::to_underlying(level) < std::to_underlying(mMinLogLevel))
				return;

			auto buffer = mArena.RequestBuffer(MessageSizeLimit);
			std::memset(buffer.data(), 0, buffer.size());

			auto level_string = Enum::ToString(level);
			auto level_format_result = std::format_to_n(buffer.begin(), level_string.size() + 2, "[{}]", level_string.data());

			auto timestamp = GetCurrentTimestamp();
			auto timestamp_format_result = std::format_to_n(buffer.begin(), level_format_result.size + timestamp.used + 3, "{} ({})", buffer.data(), timestamp.buffer.data());

			auto formatted_message = GetFormatMessage(message, std::forward<Args>(args)...);
			auto format_result = std::format_to_n(buffer.begin(), MessageSizeLimit, "{}: {}\n", buffer.data(), formatted_message.buffer.data());

			{
				std::lock_guard<std::mutex> lock(mQueueMutex);
				mMessageQueue.emplace_back(buffer, format_result.size, level);
			}
			mCV.notify_one();
		}

	private:
		void ProcessQueue()
		{
			while (true) {
				std::unique_lock<std::mutex> lock(mQueueMutex);
				mCV.wait_until(lock, mNextFlush, [this] {
					return mMessageQueue.size() >= MaxMessagesBeforeFlush or mTerminate;
				});

				Flush();

				if (mTerminate and mMessageQueue.empty())
					break;
			}
		}

		void Flush()
		{
			{
				std::vector<std::jthread> writers;
				writers.reserve(mLogTargets.size());
				for (auto const& target : mLogTargets)
				{
					auto worker = [&] { target->WriteTarget(mMessageQueue); };
					writers.emplace_back(worker);
				}
			}

			for (auto const& entry : mMessageQueue)
				mArena.ReleaseBuffer(entry.message);

			mMessageQueue.clear();
			mNextFlush = Clock::now() + MaxTimeBetweenFlush;
		}

		template<typename... Args>
		TemporaryBuffer GetFormatMessage(std::format_string<Args...> message, Args&&... args)
		{
			TemporaryBuffer temp;
			auto result = std::format_to_n(temp.buffer.data(), temp.buffer.size(), message, std::forward<Args>(args)...);
			temp.used = result.size;
			return temp;
		}

		TemporaryBuffer GetCurrentTimestamp() {
			auto now = std::chrono::system_clock::now();
			auto now_c = std::chrono::system_clock::to_time_t(now);
			auto time = GetLocalTime(&now_c);

			TemporaryBuffer temp{};
			temp.used = std::strftime(temp.buffer.data(), temp.buffer.size(), "%F %T", &time);
			return temp;
		}

	private:
		std::thread mWorker;
		std::mutex mQueueMutex;
		std::condition_variable mCV;
		TimePoint mNextFlush;
		bool mTerminate;

		LogLevel mMinLogLevel;
		std::vector<MessageEntry> mMessageQueue;
		std::vector<Impl<ILogTarget>> mLogTargets;

		LogMemoryArena mArena;
	};
}
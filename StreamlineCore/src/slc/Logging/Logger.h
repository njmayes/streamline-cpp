#pragma once

#include "LogMemoryArena.h"
#include "Targets/ILogTarget.h"

#include <thread>
#include <mutex>

namespace slc {

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

		struct LoggerStats
		{
			std::size_t messages_since_last_flush{};
			Duration time_since_last_flush{};
			std::size_t total_reallocations{};
			std::size_t large_message_count{};
		};

	public:
		Logger();
		~Logger();

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
			if (not buffer.has_value())
			{
				Flush();
				mArena.Reallocate();

				mStats.total_reallocations++;

				buffer = mArena.RequestBuffer(MessageSizeLimit);
				ASSERT(buffer.has_value(), "Still could not create message buffer despite reallocation");
			}


			std::memset(buffer.data(), 0, buffer.size());

			auto level_string = LogLevelToString(level);
			auto level_format_result = std::format_to_n(buffer.begin(), level_string.size() + 2, "[{}]", level_string.data());

			auto timestamp = GetCurrentTimestamp();
			auto timestamp_format_result = std::format_to_n(buffer.begin(), level_format_result.size + timestamp.used + 3, "{} ({})", buffer.data(), timestamp.buffer.data());

			auto formatted_message = GetFormatMessage(message, std::forward<Args>(args)...);
			auto format_result = std::format_to_n(buffer.begin(), MessageSizeLimit, "{}: {}\n", buffer.data(), formatted_message.buffer.data());

			if (format_result.size == MessageSizeLimit)
				mStats.large_message_count++;

			{
				std::lock_guard<std::mutex> lock(mQueueMutex);
				mMessageQueue.emplace_back(buffer, format_result.size, level);
			}

			mStats.messages_since_last_flush++;

			mCV.notify_one();
		}

	private:
		void ProcessQueue();
		void Flush();

		template<typename... Args>
		TemporaryBuffer GetFormatMessage(std::format_string<Args...> message, Args&&... args)
		{
			TemporaryBuffer temp;
			auto result = std::format_to_n(temp.buffer.data(), temp.buffer.size(), message, std::forward<Args>(args)...);
			temp.used = result.size;
			return temp;
		}

		TemporaryBuffer GetCurrentTimestamp();

	private:
		std::thread mWorker;
		std::mutex mQueueMutex;
		std::condition_variable mCV;
		bool mTerminate;

		TimePoint mLastFlush;

		LogLevel mMinLogLevel;
		std::vector<MessageEntry> mMessageQueue;
		std::vector<Impl<ILogTarget>> mLogTargets;

		LogMemoryArena mArena;

		LoggerStats mStats;
	};
}
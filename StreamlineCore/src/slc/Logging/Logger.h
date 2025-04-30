#pragma once

#include "LogMemoryArena.h"
#include "Targets/ILogTarget.h"

#include <thread>
#include <mutex>

namespace slc {

	class Logger
	{
	private:
		SCONSTEXPR std::size_t MessageSizeLimit = 256;
		SCONSTEXPR std::size_t MaxMessagesBeforeFlush = 1024;
		SCONSTEXPR std::size_t TemporaryBufferSize = 256;

		struct TemporaryBuffer
		{
			std::array<char, TemporaryBufferSize> buffer{};
			std::size_t used;
		};

		using Clock = std::chrono::steady_clock;
		using Duration = std::chrono::duration< float, std::micro >;
		using TimePoint = std::chrono::time_point< Clock, Duration >;

		SCONSTEXPR Duration MaxTimeBetweenFlush = Duration{ std::chrono::milliseconds(100) };

		struct LoggerStats
		{
			Duration total_pre_flush_time{};
			Duration total_write_time{};

			std::size_t total_flushes{};
			std::size_t messages_since_last_flush{};
			Duration time_since_last_flush{};
			std::size_t large_message_count{};
		};

	public:
		static Logger& GetGlobalLogger();

		Logger(std::size_t message_size_limit = MessageSizeLimit, std::size_t max_messages_before_flus = MaxMessagesBeforeFlush);
		~Logger();

		LoggerStats const& GetStats() const { return mStats; }

		template<typename target_t, typename... Args> requires
			std::derived_from<target_t, ILogTarget> and
			std::constructible_from<target_t, Args...>
			void AddLogTarget(Args&&... args)
		{
			auto target = MakeImpl<target_t>(std::forward<Args>(args)...);
			mLogTargets.push_back(std::move(target));
		}

		void Log(LogLevel level, std::string_view message);

		template<typename... Args>
		void Log(LogLevel level, std::format_string<Args...> message, Args&&... args)
		{
			if (std::to_underlying(level) < std::to_underlying(mMinLogLevel))
				return;

			auto buffer = mArena.RequestBuffer(mMessageSizeLimit);
			if (not buffer.has_value())
			{
				Flush();

				buffer = mArena.RequestBuffer(mMessageSizeLimit);
				ASSERT(buffer.has_value(), "Still could not create message buffer despite flush");
			}

			std::memset(buffer->data(), 0, buffer->size());

			auto level_string = Enum::ToString(level);
			auto level_format_result = std::format_to_n(buffer->begin(), level_string.size() + 2, "[{}]", level_string.data());

			auto timestamp = GetCurrentTimestamp();
			auto timestamp_format_result = std::format_to_n(buffer->begin(), level_format_result.size + timestamp.used + 3, "{} ({})", buffer->data(), timestamp.buffer.data());

			auto formatted_message = GetFormatMessage(message, std::forward<Args>(args)...);
			auto format_result = std::format_to_n(buffer->begin(), mMessageSizeLimit, "{}: {}\n", buffer->data(), formatted_message.data());
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

	private:
		void ProcessQueue();
		void Flush();

		template<typename... Args>
		MessageBuffer GetFormatMessage(std::format_string<Args...> message, Args&&... args)
		{
			MessageBuffer temp;
			auto result = std::format_to_n(temp.data(), temp.size(), message, std::forward<Args>(args)...);
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
		std::vector<char> mMessageBuffer;

		LoggerStats mStats;

		std::size_t mMessageSizeLimit;
		std::size_t mMaxMessagesBeforeFlush;
	};
}
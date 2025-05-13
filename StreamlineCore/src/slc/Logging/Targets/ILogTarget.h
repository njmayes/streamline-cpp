#pragma once

#include "../Common.h"

#include <vector>

namespace slc {

	class ILogTarget
	{
	public:
		ILogTarget(LogLevel level)
			: mLogLevel{ level }
		{
		}
		virtual ~ILogTarget() = default;

		void WriteTarget(std::span<MessageEntry> data, std::vector<char>& buffer);

		void PreFlush(std::vector<char>& buffer)
		{
			DoPreFlush(buffer);
		}

		void Flush()
		{
			DoFlush();
		}

		void SetLogLevel(LogLevel level) { mLogLevel = level; }
		LogLevel GetLogLevel() const { return mLogLevel; }

	private:
		virtual void DoWriteTarget(std::vector<char> const& buffer, std::size_t count) = 0;
		virtual void DoPreFlush(std::vector<char>& buffer) = 0;
		virtual void DoFlush() = 0;

		bool ShouldWriteMessage(MessageEntry const& entry) const
		{
			return std::to_underlying(entry.level) >= std::to_underlying(mLogLevel);
		}

	private:
		LogLevel mLogLevel;
	};
}
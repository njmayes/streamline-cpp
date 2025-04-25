#pragma once

#include "Common.h"

namespace slc {

	class ILogTarget
	{
	public:
		ILogTarget(LogLevel level)
			: mLogLevel{ level }
		{
		}

		void WriteTarget(MessageEntry const& entry)
		{
			if (not ShouldWriteMessage(entry))
				return;

			DoWriteTarget(entry);
		}

		void Flush()
		{
			DoFlush();
		}

		void SetLogLevel(LogLevel level) { mLogLevel = level; }
		LogLevel GetLogLevel() const { return mLogLevel; }

	private:
		virtual void DoWriteTarget(MessageEntry const& entry) = 0;
		virtual void DoFlush() = 0;

		bool ShouldWriteMessage(MessageEntry const& entry)
		{
			return std::to_underlying(entry.level) >= std::to_underlying(mLogLevel);
		}

	private:
		LogLevel mLogLevel;
	};
}
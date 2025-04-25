#pragma once

#include "slc/Common/Application.h"

namespace slc::Log {

	template<typename... Args>
	static void Trace(std::format_string<Args...> message, Args&&... args)
	{
		auto& logger = Application::GetLogger();
		logger.Log(LogLevel::Trace, message, std::forward<Args>(args)...);
	}

	template<typename... Args>
	static void Debug(std::format_string<Args...> message, Args&&... args)
	{
		auto& logger = Application::GetLogger();
		logger.Log(LogLevel::Debug, message, std::forward<Args>(args)...);
	}

	template<typename... Args>
	static void Info(std::format_string<Args...> message, Args&&... args)
	{
		auto& logger = Application::GetLogger();
		logger.Log(LogLevel::Info, message, std::forward<Args>(args)...);
	}

	template<typename... Args>
	static void Warn(std::format_string<Args...> message, Args&&... args)
	{
		auto& logger = Application::GetLogger();
		logger.Log(LogLevel::Warning, message, std::forward<Args>(args)...);
	}

	template<typename... Args>
	static void Error(std::format_string<Args...> message, Args&&... args)
	{
		auto& logger = Application::GetLogger();
		logger.Log(LogLevel::Error, message, std::forward<Args>(args)...);
	}

	template<typename... Args>
	static void Fatal(std::format_string<Args...> message, Args&&... args)
	{
		auto& logger = Application::GetLogger();
		logger.Log(LogLevel::Fatal, message, std::forward<Args>(args)...);
	}
}
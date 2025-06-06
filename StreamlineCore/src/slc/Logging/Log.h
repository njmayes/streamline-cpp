#pragma once

#include "Logger.h"

namespace slc::Log {

	/*
	 *   Utility Functions
	 */

	inline void SetGlobalLogLevel( LogLevel level )
	{
		auto& logger = Logger::GetGlobalLogger();
		logger.SetLogLevel( level );
	}


	/*
	 *	Logging Functions
	 */

	inline void Trace( std::string_view message )
	{
		auto& logger = Logger::GetGlobalLogger();
		logger.Log( LogLevel::Trace, message );
	}

	template < typename... Args >
	void Trace( std::format_string< Args... > message, Args&&... args )
	{
		auto& logger = Logger::GetGlobalLogger();
		logger.Log( LogLevel::Trace, message, std::forward< Args >( args )... );
	}

	inline void Debug( std::string_view message )
	{
		auto& logger = Logger::GetGlobalLogger();
		logger.Log( LogLevel::Debug, message );
	}

	template < typename... Args >
	void Debug( std::format_string< Args... > message, Args&&... args )
	{
		auto& logger = Logger::GetGlobalLogger();
		logger.Log( LogLevel::Debug, message, std::forward< Args >( args )... );
	}

	inline void Info( std::string_view message )
	{
		auto& logger = Logger::GetGlobalLogger();
		logger.Log( LogLevel::Info, message );
	}

	template < typename... Args >
	void Info( std::format_string< Args... > message, Args&&... args )
	{
		auto& logger = Logger::GetGlobalLogger();
		logger.Log( LogLevel::Info, message, std::forward< Args >( args )... );
	}

	inline void Warn( std::string_view message )
	{
		auto& logger = Logger::GetGlobalLogger();
		logger.Log( LogLevel::Warning, message );
	}

	template < typename... Args >
	static void Warn( std::format_string< Args... > message, Args&&... args )
	{
		auto& logger = Logger::GetGlobalLogger();
		logger.Log( LogLevel::Warning, message, std::forward< Args >( args )... );
	}

	inline void Error( std::string_view message )
	{
		auto& logger = Logger::GetGlobalLogger();
		logger.Log( LogLevel::Error, message );
	}

	template < typename... Args >
	static void Error( std::format_string< Args... > message, Args&&... args )
	{
		auto& logger = Logger::GetGlobalLogger();
		logger.Log( LogLevel::Error, message, std::forward< Args >( args )... );
	}

	inline void Fatal( std::string_view message )
	{
		auto& logger = Logger::GetGlobalLogger();
		logger.Log( LogLevel::Fatal, message );
	}

	template < typename... Args >
	static void Fatal( std::format_string< Args... > message, Args&&... args )
	{
		auto& logger = Logger::GetGlobalLogger();
		logger.Log( LogLevel::Fatal, message, std::forward< Args >( args )... );
	}
} // namespace slc::Log
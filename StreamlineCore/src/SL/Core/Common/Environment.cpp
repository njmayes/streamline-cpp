#include "Environment.h"

#include "Base.h"

#include "SL/Core/Logging/Log.h"

namespace sl::env {

	bool SetVar( std::string_view env_name, std::string_view env_val )
	{
#ifdef SLC_PLATFORM_WINDOWS
		int error = _putenv_s( env_name.data(), env_val.data() );
#elif defined( SLC_PLATFORM_LINUX )
		int error = setenv( env_name.data(), env_val.data(), 1 );
#else
		int error = -1;
#endif
		if ( error )
		{
			log::Error( "Could not set the environment variable \"{}\"", env_name );
			return false;
		}

		return true;
	}

	std::optional< std::string > GetVar( std::string_view env_name )
	{
#ifdef SLC_PLATFORM_WINDOWS
		int error;
		std::size_t required_size = 0;
		error = getenv_s( &required_size, nullptr, 0, env_name.data() );

		if ( error )
		{
			log::Error( "Could not get the environment variable \"{}\"", env_name );
			return {};
		}

		std::string result( required_size, '\0' );
		error = getenv_s( &required_size, result.data(), result.size(), env_name.data() );

		if ( error )
		{
			log::Error( "Could not get the environment variable \"{}\"", env_name );
			return {};
		}

		return result;
#elif defined( SLC_PLATFORM_LINUX )
    	const char* val = std::getenv(env_name.data());
		return val ? std::optional<std::string>{val} : std::nullopt;
#else
		return {}};
#endif
	}
} // namespace sl::env
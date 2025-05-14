#include "Environment.h"

#include "Base.h"

#include "slc/Logging/Log.h"

namespace slc::Environment {

	void SetVar(std::string_view envName, std::string_view envVal)
	{
#ifdef SLC_PLATFORM_WINDOWS
		int error = _putenv_s(envName.data(), envVal.data());
#elif defined(SLC_PLATFORM_LINUX)
		int error = setenv(envName.data(), envVal.data(), 1);
#else
		int error = -1;
#endif
		if (error)
		{
			Log::Error("Could not set the environment variable!");
		}
	}

	std::string GetVar(std::string_view envName)
	{
		errno_t error;
		std::size_t required_size = 0;
		error = getenv_s(&required_size, nullptr, 0, envName.data());

		if (error)
		{
			Log::Error("Could not get the environment variable");
			return {};
		}

		std::string result('\0', required_size);
		error = getenv_s(&required_size, result.data(), result.size(), envName.data());

		if (error)
		{
			Log::Error("Could not get the environment variable");
			return {};
		}

		return result;
	}
}
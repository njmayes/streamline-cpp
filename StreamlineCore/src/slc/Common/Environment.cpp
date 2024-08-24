#include "Environment.h"

#include "Base.h"

namespace slc {

	void Environment::SetVar(std::string_view envName, std::string_view envVal)
	{
#ifdef SLC_PLATFORM_WINDOWS
		int error = _putenv_s(envName.data(), envVal.data());
#elif defined(SLC_PLATFORM_LINUX)
		int error = setenv(envName.data(), envVal.data(), 1);
#else
		int error = -1;
#endif
		if (error)
			LOG("Could not set the environment variable!");
	}

    std::string_view Environment::GetVar(std::string_view envName) { return std::getenv(envName.data()); }
}
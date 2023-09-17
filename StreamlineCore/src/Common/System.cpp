#include "pch.h"
#include "System.h"

#include "Base.h"

namespace slc {

	void System::SetEnv(std::string_view envName, std::string_view envVal)
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

    std::string_view System::GetEnv(std::string_view envName) { return std::getenv(envName.data()); }
}
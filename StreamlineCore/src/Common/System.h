#pragma once

#include <string_view>

namespace slc {

	class System
	{
	public:
		static void SetEnv(std::string_view envName, std::string_view envVal);
		static std::string_view GetEnv(std::string_view envName);
	};
}
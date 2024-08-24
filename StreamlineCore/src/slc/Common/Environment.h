#pragma once

#include <string_view>

namespace slc {

	class Environment
	{
	public:
		static void SetVar(std::string_view envName, std::string_view envVal);
		static std::string_view GetVar(std::string_view envName);
	};
}
#pragma once

#include <string_view>
#include <string>

namespace slc::Environment {

	void SetVar(std::string_view envName, std::string_view envVal);
	std::string GetVar(std::string_view envName);
}
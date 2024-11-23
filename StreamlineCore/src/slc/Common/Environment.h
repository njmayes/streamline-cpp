#pragma once

#include <string_view>

namespace slc::Environment {

	void SetVar(std::string_view envName, std::string_view envVal);
	std::string_view GetVar(std::string_view envName);
}
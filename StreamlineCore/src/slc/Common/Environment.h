#pragma once

#include <string_view>
#include <string>
#include <optional>

namespace slc::env {

	bool SetVar( std::string_view env_name, std::string_view env_val );
	std::optional< std::string > GetVar( std::string_view env_name );
} // namespace slc::env
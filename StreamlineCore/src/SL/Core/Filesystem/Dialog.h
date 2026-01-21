#pragma once

#include <SL/Core/Common/Base.h>

namespace sl::fs {

	// These return empty if cancelled
	std::optional< std::filesystem::path > OpenFileDialog( std::vector< std::string > const& filter );
	std::optional< std::filesystem::path > OpenDirDialog();
	std::optional< std::filesystem::path > SaveFileDialog( std::vector< std::string > const& filter );
}
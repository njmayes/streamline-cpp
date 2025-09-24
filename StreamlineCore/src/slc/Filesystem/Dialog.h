#pragma once

#include <slc/Common/Base.h>

namespace slc::fs {

	// These return empty if cancelled
	std::optional< std::filesystem::path > OpenFileDialog( const std::vector< std::string >& filter );
	std::optional< std::filesystem::path > OpenDirDialog();
	std::optional< std::filesystem::path > SaveFileDialog( const std::vector< std::string >& filter );
}
#pragma once

#include <SL/Core/Common/Base.h>

namespace sl::fs {

	// These return empty if cancelled
	std::optional< std::filesystem::path > OpenFileDialog( std::vector< std::string > filter );
	std::optional< std::filesystem::path > OpenDirDialog();
	std::optional< std::filesystem::path > SaveFileDialog( std::vector< std::string > filter );


	using FileCallback = std::function< void( std::filesystem::path ) >;

	void OpenFileDialogAsync( std::vector< std::string > filter, FileCallback callback = {} );
	void OpenDirDialogAsync( FileCallback callback = {} );
	void SaveFileDialogAsync( std::vector< std::string > filter, FileCallback callback = {} );
} // namespace sl::fs
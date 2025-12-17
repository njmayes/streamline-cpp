#include "Dialog.h"

#include <portable-file-dialogs.h>

namespace sl::fs {

	std::optional< std::filesystem::path > OpenFileDialog( const std::vector< std::string >& filter )
	{
		auto selection = pfd::open_file( "Select a file", ".", filter ).result();
		if ( selection.empty() )
			return {};

		return selection[ 0 ];
	}

	std::optional< std::filesystem::path > OpenDirDialog()
	{
		auto selection = pfd::select_folder( "Select a folder", "." ).result();
		if ( selection.empty() )
			return {};

		return selection;
	}

	std::optional< std::filesystem::path > SaveFileDialog( const std::vector< std::string >& filter )
	{
		auto selection = pfd::save_file( "Save file as", ".", filter ).result();
		if ( selection.empty() )
			return {};

		return selection;
	}
}
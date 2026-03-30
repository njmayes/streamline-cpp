#include "Dialog.h"

#include "SL/Core/Common/Application.h"

#include <portable-file-dialogs.h>

namespace sl::fs {

	std::optional< std::filesystem::path > OpenFileDialog( std::vector< std::string > filter )
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

	std::optional< std::filesystem::path > SaveFileDialog( std::vector< std::string > filter )
	{
		auto selection = pfd::save_file( "Save file as", ".", filter ).result();
		if ( selection.empty() )
			return {};

		return selection;
	}

	void OpenFileDialogAsync( std::vector< std::string > filter, FileCallback callback )
	{
		std::thread(
			[ filter = std::move( filter ), callback = std::move( callback ) ]() mutable {
				auto selection = pfd::open_file( "Select a file", ".", filter ).result();

				if ( selection.empty() )
					return;

				auto path = std::filesystem::path{ selection[ 0 ] };
				if ( Application::Exists() )
				{
					Application::SubmitActionToMainThread( [ callback = std::move( callback ), path = std::move( path ) ] {
						callback( path );
					} );
				}
				else
				{
					callback( path );
				}
			}
		).detach();
	}

	void OpenDirDialogAsync( FileCallback callback )
	{
		std::thread(
			[ callback = std::move( callback ) ]() mutable {
				auto selection = pfd::select_folder( "Select a folder", "." ).result();

				if ( selection.empty() )
					return;

				auto path = std::filesystem::path{ selection };

				if ( Application::Exists() )
				{
					Application::SubmitActionToMainThread( [ callback = std::move( callback ), path = std::move( path ) ] {
						callback( path );
					} );
				}
				else
				{
					callback( path );
				}
			}
		).detach();
	}

	void SaveFileDialogAsync( std::vector< std::string > filter, FileCallback callback )
	{
		std::thread(
			[ filter = std::move( filter ), callback = std::move( callback ) ]() mutable {
				auto selection = pfd::save_file( "Save file as", ".", filter ).result();

				if ( selection.empty() )
					return;

				auto path = std::filesystem::path{ selection };
				if ( Application::Exists() )
				{
					Application::SubmitActionToMainThread( [ callback = std::move( callback ), path = std::move( path ) ] {
						callback( path );
					} );
				}
				else
				{
					callback( path );
				}
			}
		).detach();
	}
} // namespace sl::fs
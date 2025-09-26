#include "PanelManager.h"

#include "slc/ImGui/Widgets.h"
#include "slc/Logging/Log.h"

namespace slc {

	PanelManager::PanelEntry* PanelManager::Find( std::string_view key )
	{
		return &( *std::ranges::find_if( mEditorPanels, [ key ]( const auto& panel ) { return panel.init_data.key == key; } ) );
	}

	bool PanelManager::Contains( std::string_view key )
	{
		return std::ranges::find_if( mEditorPanels, [ & ]( const PanelEntry& entry ) { return entry.init_data.key == key; } ) != mEditorPanels.end();
	}

	void PanelManager::Delete( std::string_view key )
	{
		if ( !Contains( key ) )
		{
			log::Error( "EditorPanel was not registered with manager" );
			return;
		}

		std::erase_if( mEditorPanels, [ & ]( const PanelEntry& entry ) { return entry.init_data.key == key; } );
	}

	void PanelManager::Render()
	{
		for ( PanelEntry& panel_entry : mEditorPanels | std::views::filter( []( const PanelEntry& entry ) { return entry.panel && entry.displayed; } ) )
		{
			Widgets::BeginWindow( panel_entry.init_data.key );
			panel_entry.panel->OnOverlayRender();
			Widgets::EndWindow();
		}
	}
} // namespace Laby
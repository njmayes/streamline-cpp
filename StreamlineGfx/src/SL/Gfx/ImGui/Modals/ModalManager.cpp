#include "ModalManager.h"

#include "SL/Gfx/ImGui/Widgets.h"

namespace sl {

	void ModalManager::Render()
	{
		float line_spacing = Utils::FrameHeightWithSpacing();

		Utils::SetWindowMoveFromTitleBar();
		for ( ModalEntry& modal_data : mEditorModals )
		{
			if ( !modal_data.modal )
			{
				modal_data.open = false;
				continue;
			}

			Utils::SetNextWindowSize( modal_data.init_data.size );
			Utils::SetNextWindowPos( Utils::GetMainWindowCentre< Vec2f >(), Vec2f{ 0.5f, 0.5f } );
			if ( Widgets::BeginWindow( modal_data.init_data.heading, ImGuiWindowFlags_NoDocking, &modal_data.open ) )
			{
				Widgets::BeginChild( "ModalBody", Vec2f{ 0, -modal_data.init_data.button_size.y - 5.0f } );
				sl::Utils::SetWindowFontScale( modal_data.init_data.font_scale );
				modal_data.modal->OnOverlayRender();
				Widgets::EndChild();

				Widgets::BeginChild( "ModalButtons" );
				RenderButtons( modal_data );
				Widgets::EndChild();
			}

			Widgets::EndWindow();
		}
		Utils::SetWindowMoveFromTitleBar( false );

		// Call any completion callbacks before deleting modal entries. Filter for modals that are now closed and have callbacks.
		auto has_callbacks = mEditorModals |
							 std::views::filter( [ this ]( const ModalEntry& entry ) { return !entry.open && mModalCallbacks.contains( entry.init_data.heading ); } );

		for ( const ModalEntry& entry : has_callbacks )
		{
			for ( auto func : mModalCallbacks[ entry.init_data.heading ] )
				func();

			mModalCallbacks.erase( entry.init_data.heading );
		}

		std::erase_if( mEditorModals, []( const ModalEntry& entry ) { return !entry.open; } );
	}

	void ModalManager::RenderButtons( ModalEntry& modal_data )
	{
		if ( modal_data.init_data.button_type == ModalButtons::None )
			return;

		Widgets::Separator();

		Utils::SetWindowFontScale( modal_data.init_data.font_scale );

		switch ( modal_data.init_data.button_type )
		{
			case ModalButtons::OK:
			{
				Widgets::Button( "OK", modal_data.init_data.button_size, [ & ]() {
					modal_data.modal->OnComplete();
					modal_data.open = false;
				} );
				break;
			}
			case ModalButtons::OKCancel:
			{
				Widgets::Button( "OK", modal_data.init_data.button_size, [ & ]() {
					modal_data.modal->OnComplete();
					modal_data.open = false;
				} );

				Widgets::SameLine();

				Widgets::Button( "Cancel", modal_data.init_data.button_size, [ & ]() { modal_data.open = false; } );

				break;
			}
			case ModalButtons::YesNo:
			{
				Widgets::Button( "Yes", modal_data.init_data.button_size, [ & ]() {
					modal_data.modal->OnComplete();
					modal_data.open = false;
				} );

				Widgets::SameLine();

				Widgets::Button( "No", modal_data.init_data.button_size, [ & ]() { modal_data.open = false; } );

				break;
			}
			case ModalButtons::Custom:
				modal_data.modal->OnCustomButtonRender( modal_data.open );
				break;
		}
	}

} // namespace sl
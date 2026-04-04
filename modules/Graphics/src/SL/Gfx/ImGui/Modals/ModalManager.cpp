#include "ModalManager.h"

#include "SL/Gfx/ImGui/Widgets.h"

namespace sl {

	void ModalManager::Render()
	{
		float line_spacing = Utils::FrameHeightWithSpacing();

		Utils::SetWindowMoveFromTitleBar();

		for ( ModalEntry& modal_data : mEditorModals )
		{
			Utils::SetNextWindowSize( modal_data.init_data.size );
			Utils::SetNextWindowPos( Utils::GetMainWindowCentre< Vec2f >(), Vec2f{ 0.5f, 0.5f } );
			if ( Widgets::BeginWindow( modal_data.init_data.heading, ImGuiWindowFlags_NoDocking, nullptr ) )
			{
				Widgets::BeginChild( "ModalBody", Vec2f{ 0, -modal_data.init_data.button_size.y - 8.0f } );
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

		// Call any completion callbacks before deleting modal entries. Filter for modals that are now closed.
		auto has_closed = mEditorModals | std::views::filter( [ this ]( const ModalEntry& entry ) { return !entry.open; } );

		for ( ModalEntry& entry : has_closed )
		{
			for ( auto const& func : entry.modal->mCompletionCallbacks )
				func();

			entry.modal->OnClose();
		}

		std::erase_if( mEditorModals, []( const ModalEntry& entry ) { return !entry.open; } );

		if ( mBlockEsc and std::ranges::count_if( mEditorModals, []( const ModalEntry& entry ) { return entry.open and entry.init_data.block_exit; } ) == 0 )
		{
			mBlockEsc = false;
			Application::BlockEsc( mBlockEsc );
		}
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
				modal_data.modal->OnCustomButtonRender( modal_data.open, modal_data.init_data.button_size );
				break;
		}
	}

} // namespace sl
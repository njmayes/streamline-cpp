#include "ModalManager.h"

#include "slc/Common/Application.h"
#include "slc/ImGui/Widgets.h"

namespace slc {

	static auto constexpr ButtonSize = Vec2f{ 60, 40 };

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

			Utils::SetNextWindowSize< glm::vec2 >( { 600, 400 } );
			Utils::SetNextWindowPos< glm::vec2 >( Utils::GetMainWindowCentre< glm::vec2 >(), { 0.5f, 0.5f } );
			if ( Widgets::BeginWindow( modal_data.heading, &modal_data.open, ImGuiWindowFlags_NoDocking ) )
			{
				Widgets::BeginChild( "ModalBody", glm::vec2{ 0, -ButtonSize.y - 5.0f } );
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
							 std::views::filter( [ this ]( const ModalEntry& entry ) { return !entry.open && mModalCallbacks.contains( entry.heading ); } );

		for ( const ModalEntry& entry : has_callbacks )
		{
			for ( auto func : mModalCallbacks[ entry.heading ] )
				func();

			mModalCallbacks.erase( entry.heading );
		}

		std::erase_if( mEditorModals, []( const ModalEntry& entry ) { return !entry.open; } );
	}

	void ModalManager::RenderButtons( ModalEntry& modal_data )
	{
		if ( modal_data.type == ModalButtons::None )
			return;

		Widgets::Separator();

		Utils::SetWindowFontScale( 2.0f );

		switch ( modal_data.type )
		{
			case ModalButtons::OK:
			{
				Widgets::Button( "OK", ButtonSize, [ & ]() {
					modal_data.modal->OnComplete();
					modal_data.open = false;
				} );
				break;
			}
			case ModalButtons::OKCancel:
			{
				Widgets::Button( "OK", ButtonSize, [ & ]() {
					modal_data.modal->OnComplete();
					modal_data.open = false;
				} );

				Widgets::SameLine();

				Widgets::Button( "Cancel", ButtonSize, [ & ]() { modal_data.open = false; } );

				break;
			}
			case ModalButtons::YesNo:
			{
				Widgets::Button( "Yes", ButtonSize, [ & ]() {
					modal_data.modal->OnComplete();
					modal_data.open = false;
				} );

				Widgets::SameLine();

				Widgets::Button( "No", ButtonSize, [ & ]() { modal_data.open = false; } );

				break;
			}
			case ModalButtons::Custom:
				modal_data.modal->OnCustomButtonRender( modal_data.open );
				break;
		}
	}

} // namespace slc
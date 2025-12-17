#include "Utils.h"

#include "imgui.h"
#include "imgui_internal.h"

namespace sl {

	bool Utils::IsKeyPressed( KeyCode key )
	{
		ImGuiIO io = ImGui::GetIO();
		return io.KeysData[ key ].Down;
	}

	bool Utils::IsMouseDown( MouseCode button )
	{
		return ImGui::IsMouseDown( button );
	}

	bool Utils::IsMouseReleased( MouseCode button )
	{
		return ImGui::IsMouseReleased( button );
	}

	ImVec2 Utils::CursorPosInternal()
	{
		return ImGui::GetCursorPos();
	}

	ImVec2 Utils::MousePos()
	{
		return ImGui::GetMousePos();
	}

	ImVec2 Utils::WindowPos()
	{
		return ImGui::GetWindowPos();
	}

	void Utils::SetCursorPos( float x, float y )
	{
		ImGui::SetCursorPos( ImVec2( x, y ) );
	}

	void Utils::SetCursorPosX( float pos )
	{
		ImGui::SetCursorPosX( pos );
	}

	void Utils::SetCursorPosY( float pos )
	{
		ImGui::SetCursorPosY( pos );
	}

	void Utils::SetNextWindowSizeInternal( const ImVec2& size, ImGuiCond cond )
	{
		ImGui::SetNextWindowSize( size, cond );
	}

	void Utils::SetNextWindowPosInternal( const ImVec2& size, const ImVec2& pivot, ImGuiCond cond )
	{
		ImGui::SetNextWindowPos( size, cond, pivot );
	}

	void Utils::SetButtonColourInternal( const ImVec4& colour )
	{
		ImGui::PushStyleColor( ImGuiCol_Button, colour );
	}

	void Utils::SetButtonDefaults()
	{
		auto& colours = ImGui::GetStyle().Colors;
		const auto& button_hovered = colours[ ImGuiCol_ButtonHovered ];
		const auto& button_active = colours[ ImGuiCol_ButtonActive ];

		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( button_hovered.x, button_hovered.y, button_hovered.z, 0.5f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( button_active.x, button_active.y, button_active.z, 0.5f ) );
	}

	ImVec2 Utils::GetMainWindowCentreInternal()
	{
		return ImGui::GetMainViewport()->GetCenter();
	}

	void Utils::SetWindowMoveFromTitleBar( bool titleBarOnly )
	{
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigWindowsMoveFromTitleBarOnly = titleBarOnly;
	}

	ImVec2 Utils::AvailableRegion()
	{
		return ImGui::GetContentRegionAvail();
	}

	ImVec2 Utils::AvailableRegionMin()
	{
		return ImGui::GetWindowContentRegionMin();
	}

	ImVec2 Utils::AvailableRegionMax()
	{
		return ImGui::GetWindowContentRegionMax();
	}

	float Utils::FontSize()
	{
		return GImGui->Font->LegacySize;
	}

	ImVec2 Utils::FramePadding()
	{
		return GImGui->Style.FramePadding;
	}

	float Utils::FrameHeightWithSpacing()
	{
		return ImGui::GetFrameHeightWithSpacing();
	}

	float Utils::LineHeight()
	{
		return GImGui->Font->LegacySize + GImGui->Style.FramePadding.y * 2.0f;
	}

	float Utils::WindowWidth()
	{
		return ImGui::GetWindowWidth();
	}

	float Utils::WindowHeight()
	{
		return ImGui::GetWindowHeight();
	}

	void Utils::SetWindowFontScale( float scale )
	{
		ImGui::SetWindowFontScale( scale );
	}

	bool Utils::ItemFocused()
	{
		return ImGui::IsItemFocused();
	}

	bool Utils::ItemHovered()
	{
		return ImGui::IsItemHovered();
	}

	bool Utils::WindowFocused()
	{
		return ImGui::IsWindowFocused();
	}

	bool Utils::WindowHovered()
	{
		return ImGui::IsWindowHovered();
	}

	void Utils::PushItemWidth( float width )
	{
		ImGui::PushItemWidth( width );
	}

	void Utils::PopItemWidth()
	{
		ImGui::PopItemWidth();
	}

	void Utils::PushID( std::string_view strID )
	{
		ImGui::PushID( strID.data() );
	}

	void Utils::PopID()
	{
		ImGui::PopID();
	}

	void Utils::PushStyle( ImGuiStyleVar flags, float var )
	{
		ImGui::PushStyleVar( flags, var );
	}

	void Utils::PushStyle( ImGuiStyleVar flags, const ImVec2& var )
	{
		ImGui::PushStyleVar( flags, var );
	}

	void Utils::PopStyle( int count )
	{
		ImGui::PopStyleVar( count );
	}

	void Utils::PushStyleColour( ImGuiCol flags, const ImVec4& var )
	{
		ImGui::PushStyleColor( flags, var );
	}

	void Utils::PopStyleColour( int count )
	{
		ImGui::PopStyleColor( count );
	}

	bool Utils::IsLeftMouseClicked()
	{
		return ImGui::IsMouseClicked( ImGuiMouseButton_Left );
	}

	bool Utils::IsLeftMouseDoubleClicked()
	{
		return ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left );
	}

	void Utils::ReloadCurrentBuffer()
	{
		if ( ImGuiInputTextState * state{ ImGui::GetInputTextState( ImGui::GetItemID() ) } )
			state->ReloadUserBufAndSelectAll();
	}

	void Utils::SetKeyboardFocusHere()
	{
		ImGui::SetKeyboardFocusHere( -1 );
	}
} // namespace sl
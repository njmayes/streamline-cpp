#include "Widgets.h"

#include "slc/Types/StaticString.h"

#include "imgui.h"
#include "imgui_internal.h"

namespace slc {

	void Widgets::NewLine()
	{
		ImGui::NewLine();
	}

	void Widgets::SameLine( float xOffset, float spacing )
	{
		ImGui::SameLine( xOffset, spacing );
	}

	void Widgets::Separator()
	{
		ImGui::Separator();
	}

	void Widgets::Spacing()
	{
		ImGui::Spacing();
	}

	void Widgets::Disable( bool disable )
	{
		ImGui::BeginDisabled( disable );
	}

	void Widgets::EndDisable()
	{
		ImGui::EndDisabled();
	}

	void Widgets::SetXPosition( float pos )
	{
		ImGui::SetCursorPosX( pos );
	}

	void Widgets::SetYPosition( float pos )
	{
		ImGui::SetCursorPosY( pos );
	}

	void Widgets::BeginDockspace()
	{
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
		static bool dockspace_open = true;
		static bool opt_fullscreen_persistant = true;
		bool opt_fullscreen = opt_fullscreen_persistant;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if ( opt_fullscreen )
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos( viewport->Pos );
			ImGui::SetNextWindowSize( viewport->Size );
			ImGui::SetNextWindowViewport( viewport->ID );
			ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.0f );
			ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		if ( dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode )
			window_flags |= ImGuiWindowFlags_NoBackground;

		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );
		ImGui::Begin( "DockSpace Demo", &dockspace_open, window_flags );
		ImGui::PopStyleVar();

		if ( opt_fullscreen )
			ImGui::PopStyleVar( 2 );

		// DockSpace
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		float min_win_size_x = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;

		if ( io.ConfigFlags & ImGuiConfigFlags_DockingEnable )
		{
			ImGuiID dockspace_id = ImGui::GetID( "MyDockSpace" );
			ImGui::DockSpace( dockspace_id, ImVec2( 0.0f, 0.0f ), dockspace_flags );
		}

		style.WindowMinSize.x = min_win_size_x;
	}

	void Widgets::EndDockspace()
	{
		ImGui::End();
	}

	bool Widgets::BeginWindow( std::string_view heading, ImGuiWindowFlags flags, bool* open )
	{
		return ImGui::Begin( heading.data(), open, flags );
	}

	void Widgets::EndWindow()
	{
		ImGui::End();
	}

	void Widgets::BeginColumns( int count, bool border )
	{
		ImGui::Columns( count, 0, border );
	}

	void Widgets::NextColumn()
	{
		ImGui::NextColumn();
	}

	void Widgets::EndColumns()
	{
		ImGui::Columns( 1 );
	}

	void Widgets::BeginChildInternal( std::string_view strID, const ImVec2& size, bool border )
	{
		ImGui::BeginChild( strID.data(), size, border );
	}

	void Widgets::EndChild()
	{
		ImGui::EndChild();
	}

	void Widgets::BeginGroup()
	{
		ImGui::BeginGroup();
	}

	void Widgets::EndGroup()
	{
		ImGui::EndGroup();
	}

	bool Widgets::SelectableInternal( std::string_view label, bool selected )
	{
		return ImGui::Selectable( label.data(), selected );
	}

	bool Widgets::TreeNodeEx( void* id, std::string_view text, ImGuiTreeNodeFlags flags )
	{
		return ImGui::TreeNodeEx( id, flags, "%s", text.data() );
	}

	bool Widgets::TreeNodeExInternal( void* id, std::string_view text, bool selected )
	{
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		flags |= selected ? ImGuiTreeNodeFlags_Selected : 0;
		return ImGui::TreeNodeEx( ( void* )&id, flags, "%s", text.data() );
	}

	void Widgets::TreePopInternal()
	{
		ImGui::TreePop();
	}

	void Widgets::BeginMenuBar()
	{
		sCurrentMenuBar = ui::MenuBar();
	}

	void Widgets::AddMenuBarHeading( std::string_view heading )
	{
		sCurrentMenuBar.AddHeading( heading );
	}

	void Widgets::AddMenuBarItem( std::string_view heading, Action<>&& action )
	{
		sCurrentMenuBar.AddMenuItemAction( heading, "", std::move( action ) );
	}

	void Widgets::AddMenuBarItem( std::string_view heading, std::string_view shortcut, Action<>&& action )
	{
		sCurrentMenuBar.AddMenuItemAction( heading, shortcut, std::move( action ) );
	}

	void Widgets::AddMenuBarItem( std::string_view heading, bool& displayed )
	{
		sCurrentMenuBar.AddMenuItemSwitch( heading, "", displayed );
	}

	void Widgets::AddMenuBarSeparator()
	{
		sCurrentMenuBar.AddSeparator();
	}

	void Widgets::EndMenuBar()
	{
		sCurrentMenuBar = ui::MenuBar();
	}

	void Widgets::OpenPopup( std::string_view popupName )
	{
		ImGui::OpenPopup( popupName.data() );
	}

	void Widgets::BeginPopup( std::string_view popupName )
	{
		sCurrentPopup = ui::PopUp( popupName );
	}

	void Widgets::AddPopupItem( std::string_view heading, Action<>&& action )
	{
		sCurrentPopup.AddPopUpItem( heading, std::move( action ) );
	}

	void Widgets::EndPopup()
	{
		sCurrentPopup = ui::PopUp();
	}

	void Widgets::BeginContextPopup()
	{
		sCurrentPopupCtx = ui::PopUpContext();
	}

	void Widgets::AddContextItem( std::string_view heading, Action<>&& action )
	{
		sCurrentPopupCtx.AddPopUpItem( heading, std::move( action ) );
	}

	void Widgets::EndContextPopup()
	{
		sCurrentPopupCtx = ui::PopUpContext();
	}

	void Widgets::Label( std::string_view text )
	{
		if ( text.empty() )
		{
			ImGui::Text( "..." );
			return;
		}

		ImGui::TextUnformatted( text.data() );
	}

	void Widgets::LabelWrapped( std::string_view text )
	{
		if ( text.empty() )
		{
			ImGui::TextWrapped( "..." );
			return;
		}
		ImGui::TextWrapped( "%s", text.data() );
	}

	void Widgets::StringEdit( std::string_view label, std::string& field, ImGuiInputTextFlags flags, Action<>&& action )
	{
		flags |= ImGuiInputTextFlags_CallbackResize;

		auto callback = []( ImGuiInputTextCallbackData* data ) -> int {
			if ( data->EventFlag == ImGuiInputTextFlags_CallbackResize )
			{
				auto* my_str = ( std::string* )data->UserData;
				IM_ASSERT( my_str->c_str() == data->Buf );
				my_str->reserve( data->BufSize - 1 ); // ensure enough capacity
				my_str->resize( data->BufTextLen );	  // current text length
				data->Buf = ( char* )my_str->c_str();
			}
			return 0;
		};

		if ( ImGui::InputText( label.data(), ( char* )field.c_str(), field.capacity() + 1, flags, callback, &field ) and action )
			action();
	}

	void Widgets::PathEdit( std::string_view label, std::filesystem::path& field, ImGuiInputTextFlags flags, Action<>&& action )
	{
		std::string temp = field.string();

		Widgets::StringEdit( label, temp, flags, std::move( action ) );

		if ( temp != field.string() )
			field = temp;
	}

	bool Widgets::BeginDragDropSourceInternal()
	{
		return ImGui::BeginDragDropSource( ImGuiDragDropFlags_SourceAllowNullID );
	}


	void Widgets::EndDragDropSourceInternal( std::string_view strID, const void* data, size_t size )
	{
		ImGui::SetDragDropPayload( strID.data(), data, size );
		ImGui::EndDragDropSource();
	}

	void* Widgets::DragDropTargetInternal( std::string_view strID )
	{
		if ( !ImGui::BeginDragDropTarget() )
			return nullptr;

		const ImGuiPayload* payload = ImGui::AcceptDragDropPayload( strID.data() );
		ImGui::EndDragDropTarget();

		if ( !payload )
			return nullptr;
		return payload->Data;
	}

	bool Widgets::CheckboxInternal( std::string_view label, bool& value )
	{
		return ImGui::Checkbox( label.data(), &value );
	}

	bool Widgets::ButtonInternal( std::string_view label )
	{
		return ImGui::Button( label.data() );
	}

	bool Widgets::ButtonInternal( std::string_view label, const ImVec2& size )
	{
		return ImGui::Button( label.data(), size );
	}

	void Widgets::Vector2EditInternalRef( std::string_view label, ImVec2& values, float resetVal, float colWidth )
	{
		ImGuiIO& io = ImGui::GetIO();
		auto bold_font = io.Fonts->Fonts[ 0 ];

		ImGui::PushID( label.data() );

		ImGui::Columns( 2 );
		ImGui::SetColumnWidth( 0, colWidth );
		ImGui::TextUnformatted( label.data() );
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths( 2, ImGui::CalcItemWidth() );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 } );

		float line_height = GImGui->Font->LegacySize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 button_size = { line_height + 3.0f, line_height };

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f } );
		ImGui::PushFont( bold_font );
		if ( ImGui::Button( "X", button_size ) )
			values.x = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		ImGui::DragFloat( "##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f" );
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f } );
		ImGui::PushFont( bold_font );
		if ( ImGui::Button( "Y", button_size ) )
			values.y = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		ImGui::DragFloat( "##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f" );
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns( 1 );

		ImGui::PopID();
	}

	void Widgets::Vector3EditInternalRef( std::string_view label, ImVec3& values, float resetVal, float colWidth )
	{
		ImGuiIO& io = ImGui::GetIO();
		auto bold_font = io.Fonts->Fonts[ 0 ];

		ImGui::PushID( label.data() );

		ImGui::Columns( 2 );
		ImGui::SetColumnWidth( 0, colWidth );
		ImGui::TextUnformatted( label.data() );
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths( 3, ImGui::CalcItemWidth() );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 } );

		float line_height = GImGui->Font->LegacySize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 button_size = { line_height + 3.0f, line_height };

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f } );
		ImGui::PushFont( bold_font );
		if ( ImGui::Button( "X", button_size ) )
			values.x = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		ImGui::DragFloat( "##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f" );
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f } );
		ImGui::PushFont( bold_font );
		if ( ImGui::Button( "Y", button_size ) )
			values.y = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		ImGui::DragFloat( "##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f" );
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f } );
		ImGui::PushFont( bold_font );
		if ( ImGui::Button( "Z", button_size ) )
			values.z = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		ImGui::DragFloat( "##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f" );
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns( 1 );

		ImGui::PopID();
	}

	void Widgets::Vector4EditInternalRef( std::string_view label, ImVec4& values, float resetVal, float colWidth )
	{
		ImGuiIO& io = ImGui::GetIO();
		auto bold_font = io.Fonts->Fonts[ 0 ];

		ImGui::PushID( label.data() );

		ImGui::Columns( 2 );
		ImGui::SetColumnWidth( 0, colWidth );
		ImGui::TextUnformatted( label.data() );
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths( 4, ImGui::CalcItemWidth() );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 } );

		float line_height = GImGui->Font->LegacySize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 button_size = { line_height + 3.0f, line_height };

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f } );
		ImGui::PushFont( bold_font );
		if ( ImGui::Button( "X", button_size ) )
			values.x = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		ImGui::DragFloat( "##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f" );
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f } );
		ImGui::PushFont( bold_font );
		if ( ImGui::Button( "Y", button_size ) )
			values.y = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		ImGui::DragFloat( "##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f" );
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f } );
		ImGui::PushFont( bold_font );
		if ( ImGui::Button( "Z", button_size ) )
			values.z = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		ImGui::DragFloat( "##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f" );
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f } );
		ImGui::PushFont( bold_font );
		if ( ImGui::Button( "W", button_size ) )
			values.w = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		ImGui::DragFloat( "##W", &values.w, 0.1f, 0.0f, 0.0f, "%.2f" );
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns( 1 );

		ImGui::PopID();
	}

	ImVec2 Widgets::Vector2EditInternal( std::string_view label, ImVec2 values, float resetVal, float colWidth )
	{
		ImGuiIO& io = ImGui::GetIO();
		auto bold_font = io.Fonts->Fonts[ 0 ];

		ImGui::PushID( label.data() );

		ImGui::Columns( 2 );
		ImGui::SetColumnWidth( 0, colWidth );
		ImGui::TextUnformatted( label.data() );
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths( 2, ImGui::CalcItemWidth() );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 } );

		float line_height = GImGui->Font->LegacySize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 button_size = { line_height + 3.0f, line_height };

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f } );
		ImGui::PushFont( bold_font );
		if ( ImGui::Button( "X", button_size ) )
			values.x = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		ImGui::DragFloat( "##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f" );
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f } );
		ImGui::PushFont( bold_font );
		if ( ImGui::Button( "Y", button_size ) )
			values.y = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		ImGui::DragFloat( "##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f" );
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns( 1 );

		ImGui::PopID();

		return values;
	}

	ImVec3 Widgets::Vector3EditInternal( std::string_view label, ImVec3 values, float resetVal, float colWidth )
	{
		ImGuiIO& io = ImGui::GetIO();
		auto bold_font = io.Fonts->Fonts[ 0 ];

		ImGui::PushID( label.data() );

		ImGui::Columns( 2 );
		ImGui::SetColumnWidth( 0, colWidth );
		ImGui::TextUnformatted( label.data() );
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths( 3, ImGui::CalcItemWidth() );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 } );

		float line_height = GImGui->Font->LegacySize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 button_size = { line_height + 3.0f, line_height };

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f } );
		ImGui::PushFont( bold_font );
		if ( ImGui::Button( "X", button_size ) )
			values.x = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		ImGui::DragFloat( "##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f" );
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f } );
		ImGui::PushFont( bold_font );
		if ( ImGui::Button( "Y", button_size ) )
			values.y = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		ImGui::DragFloat( "##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f" );
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f } );
		ImGui::PushFont( bold_font );
		if ( ImGui::Button( "Z", button_size ) )
			values.z = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		ImGui::DragFloat( "##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f" );
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns( 1 );

		ImGui::PopID();

		return values;
	}

	ImVec4 Widgets::Vector4EditInternal( std::string_view label, ImVec4 values, float resetVal, float colWidth )
	{
		ImGuiIO& io = ImGui::GetIO();
		auto bold_font = io.Fonts->Fonts[ 0 ];

		ImGui::PushID( label.data() );

		ImGui::Columns( 2 );
		ImGui::SetColumnWidth( 0, colWidth );
		ImGui::TextUnformatted( label.data() );
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths( 4, ImGui::CalcItemWidth() );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 } );

		float line_height = GImGui->Font->LegacySize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 button_size = { line_height + 3.0f, line_height };

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f } );
		ImGui::PushFont( bold_font );
		if ( ImGui::Button( "X", button_size ) )
			values.x = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		ImGui::DragFloat( "##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f" );
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f } );
		ImGui::PushFont( bold_font );
		if ( ImGui::Button( "Y", button_size ) )
			values.y = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		ImGui::DragFloat( "##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f" );
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f } );
		ImGui::PushFont( bold_font );
		if ( ImGui::Button( "Z", button_size ) )
			values.z = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		ImGui::DragFloat( "##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f" );
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f } );
		ImGui::PushFont( bold_font );
		if ( ImGui::Button( "W", button_size ) )
			values.w = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		ImGui::DragFloat( "##W", &values.w, 0.1f, 0.0f, 0.0f, "%.2f" );
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns( 1 );

		ImGui::PopID();

		return values;
	}

	void Widgets::ColourEditInternal( std::string_view label, ImVec4& colour )
	{
		ImGui::ColorEdit4( label.data(), &colour.x );
	}

	void Widgets::ImageInternal( ImTextureID image, const ImVec2& size, float rotation, const ImVec2& uv0, const ImVec2& uv1 )
	{
		ImGui::Image( image, size, uv0, uv1 );
	}

	bool Widgets::ImageButtonInternal( std::string_view id, ImTextureID image, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1 )
	{
		return ImGui::ImageButton( id.data(), image, size, uv0, uv1 );
	}

	int64_t Widgets::ScalarEdit( std::string_view label, int64_t field )
	{
		int scalar = ( int )field;
		ImGui::InputInt( label.data(), &scalar );
		return ( int64_t )scalar;
	}

	uint64_t Widgets::UScalarEdit( std::string_view label, uint64_t field )
	{
		int val = ( int )field;
		ImGui::InputInt( label.data(), &val );
		if ( val < 0 )
			val = 0;
		return ( uint64_t )val;
	}

	float Widgets::FloatEditInternal( std::string_view label, float field, float speed, float min, float max )
	{
		float val = ( float )field;
		ImGui::DragFloat( label.data(), &val, speed, min, max );
		return ( float )val;
	}

	bool Widgets::BeginCombo( std::string_view label, std::string_view preview )
	{
		return ImGui::BeginCombo( label.data(), preview.data() );
	}

	bool Widgets::ComboboxEntry( std::string_view preview, const IComboEntry* entry )
	{
		bool selection_change = false;

		bool is_selected = entry->key == preview;
		if ( ImGui::Selectable( entry->key.data(), is_selected ) )
			selection_change = true;

		if ( is_selected )
			ImGui::SetItemDefaultFocus();

		return selection_change;
	}

	void Widgets::EndCombo()
	{
		ImGui::EndCombo();
	}
} // namespace slc
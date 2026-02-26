#include "PopUp.h"

#include "imgui.h"

namespace sl::ui {

	PopUp::~PopUp()
	{
		if ( mPopUpItems.empty() )
			return;

		if ( !ImGui::BeginPopup( mStrID.data() ) )
			return;

		auto selected_popups = mPopUpItems | std::views::filter( []( const auto& item ) { return ImGui::MenuItem( item.label.data() ); } );

		for ( const PopUpItem& item : selected_popups )
		{
			item.action();
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	PopUp& PopUp::AddPopUpItem( std::string_view label, Action<>&& action )
	{
		mPopUpItems.emplace_back( label, std::move( action ) );
		return *this;
	}

	PopUpContext::~PopUpContext()
	{
		if ( mPopUpItems.empty() )
			return;

		if ( !ImGui::BeginPopupContextItem() )
			return;

		auto selected_popups = mPopUpItems | std::views::filter( []( const auto& item ) { return ImGui::MenuItem( item.label.data() ); } );
		for ( const PopUpItem& item : selected_popups )
			item.action();

		ImGui::EndPopup();
	}

	PopUpContext& PopUpContext::AddPopUpItem( std::string_view label, Action<>&& action )
	{
		mPopUpItems.emplace_back( label, std::move( action ) );
		return *this;
	}
} // namespace sl::ui
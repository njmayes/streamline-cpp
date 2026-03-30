#include "MenuBar.h"

#include "imgui.h"

namespace sl::ui {
	namespace {

		template < typename MenuBarT >
		MenuBarT& AddHeadingImpl( MenuBarT& bar, std::vector< MenuHeading >& items, std::string_view heading )
		{
			items.emplace_back( heading );
			return bar;
		}

		template < typename MenuBarT >
		MenuBarT& AddMenuItemActionImpl( MenuBarT& bar, std::vector< MenuHeading >& items, std::string_view label, std::string_view shortcut, Action<>&& action )
		{
			SL_VERIFY( !items.empty(), "Menu item added without a heading" );
			MenuHeading& last_menu = items.back();
			last_menu.menu.emplace_back( MenuItemType::Action, label, shortcut, std::move( action ) );
			return bar;
		}

		template < typename MenuBarT >
		MenuBarT& AddMenuItemSwitchImpl( MenuBarT& bar, std::vector< MenuHeading >& items, std::string_view label, std::string_view shortcut, bool& show )
		{
			SL_VERIFY( !items.empty(), "Menu item added without a heading" );
			MenuHeading& last_menu = items.back();
			last_menu.menu.emplace_back( MenuItemType::Switch, label, shortcut, show );
			return bar;
		}

		template < typename MenuBarT >
		MenuBarT& AddMenuItemSelectableImpl(
			MenuBarT& bar,
			std::vector< MenuHeading >& items,
			std::string_view label,
			std::string_view shortcut,
			bool selected,
			Action<>&& action
		)
		{
			SL_VERIFY( !items.empty(), "Menu item added without a heading" );
			MenuHeading& last_menu = items.back();
			last_menu.menu.emplace_back( MenuItemType::Selectable, label, shortcut, selected, std::move( action ) );
			return bar;
		}

		template < typename MenuBarT >
		MenuBarT& AddSeparatorImpl( MenuBarT& bar, std::vector< MenuHeading >& items )
		{
			SL_VERIFY( !items.empty(), "Menu item added without a heading" );
			MenuHeading& last_menu = items.back();
			last_menu.menu.emplace_back( MenuItemType::Separator );
			return bar;
		}

		void DrawMenuContents( const std::vector< MenuHeading >& items )
		{
			auto open_menus = items | std::views::filter( []( const auto& heading ) { return ImGui::BeginMenu( heading.label.data() ); } );

			for ( const MenuHeading& heading : open_menus )
			{
				for ( const MenuItem& item : heading.menu )
				{
					switch ( item.type )
					{
						case MenuItemType::Action:
							if ( ImGui::MenuItem( item.label.data(), item.shortcut.data() ) )
								item.action();
							break;

						case MenuItemType::Switch:
							ImGui::MenuItem( item.label.data(), item.shortcut.data(), item.display );
							break;

						case MenuItemType::Selectable:
							if ( ImGui::MenuItem( item.label.data(), item.shortcut.data(), item.selected ) )
								item.action();
							break;

						case MenuItemType::Separator:
							ImGui::Separator();
							break;
					}
				}

				ImGui::EndMenu();
			}
		}

	} // namespace

	MenuBar::~MenuBar()
	{
		if ( mMenuItems.empty() )
			return;

		if ( !ImGui::BeginMenuBar() )
			return;

		DrawMenuContents( mMenuItems );

		ImGui::EndMenuBar();
	}

	MenuBar& MenuBar::AddHeading( std::string_view heading )
	{
		return AddHeadingImpl( *this, mMenuItems, heading );
	}

	MenuBar& MenuBar::AddMenuItemAction( std::string_view label, Action<>&& action )
	{
		return AddMenuItemActionImpl( *this, mMenuItems, label, "", std::move( action ) );
	}

	MenuBar& MenuBar::AddMenuItemSwitch( std::string_view label, bool& show )
	{
		return AddMenuItemSwitchImpl( *this, mMenuItems, label, "", show );
	}

	MenuBar& MenuBar::AddMenuItemSelectable( std::string_view label, bool selected, Action<>&& action )
	{
		return AddMenuItemSelectableImpl( *this, mMenuItems, label, "", selected, std::move( action ) );
	}

	MenuBar& MenuBar::AddMenuItemAction( std::string_view label, std::string_view shortcut, Action<>&& action )
	{
		return AddMenuItemActionImpl( *this, mMenuItems, label, shortcut, std::move( action ) );
	}

	MenuBar& MenuBar::AddMenuItemSwitch( std::string_view label, std::string_view shortcut, bool& show )
	{
		return AddMenuItemSwitchImpl( *this, mMenuItems, label, shortcut, show );
	}

	MenuBar& MenuBar::AddMenuItemSelectable( std::string_view label, std::string_view shortcut, bool selected, Action<>&& action )
	{
		return AddMenuItemSelectableImpl( *this, mMenuItems, label, shortcut, selected, std::move( action ) );
	}

	MenuBar& MenuBar::AddSeparator()
	{
		return AddSeparatorImpl( *this, mMenuItems );
	}

	MainMenuBar::~MainMenuBar()
	{
		if ( mMenuItems.empty() )
			return;

		if ( !ImGui::BeginMainMenuBar() )
			return;

		DrawMenuContents( mMenuItems );

		ImGui::EndMainMenuBar();
	}

	MainMenuBar& MainMenuBar::AddHeading( std::string_view heading )
	{
		return AddHeadingImpl( *this, mMenuItems, heading );
	}

	MainMenuBar& MainMenuBar::AddMenuItemAction( std::string_view label, Action<>&& action )
	{
		return AddMenuItemActionImpl( *this, mMenuItems, label, "", std::move( action ) );
	}

	MainMenuBar& MainMenuBar::AddMenuItemSwitch( std::string_view label, bool& show )
	{
		return AddMenuItemSwitchImpl( *this, mMenuItems, label, "", show );
	}

	MainMenuBar& MainMenuBar::AddMenuItemSelectable( std::string_view label, bool selected, Action<>&& action )
	{
		return AddMenuItemSelectableImpl( *this, mMenuItems, label, "", selected, std::move( action ) );
	}

	MainMenuBar& MainMenuBar::AddMenuItemAction( std::string_view label, std::string_view shortcut, Action<>&& action )
	{
		return AddMenuItemActionImpl( *this, mMenuItems, label, shortcut, std::move( action ) );
	}

	MainMenuBar& MainMenuBar::AddMenuItemSwitch( std::string_view label, std::string_view shortcut, bool& show )
	{
		return AddMenuItemSwitchImpl( *this, mMenuItems, label, shortcut, show );
	}

	MainMenuBar& MainMenuBar::AddMenuItemSelectable( std::string_view label, std::string_view shortcut, bool selected, Action<>&& action )
	{
		return AddMenuItemSelectableImpl( *this, mMenuItems, label, shortcut, selected, std::move( action ) );
	}

	MainMenuBar& MainMenuBar::AddSeparator()
	{
		return AddSeparatorImpl( *this, mMenuItems );
	}

} // namespace sl::ui
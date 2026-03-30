#pragma once

#include "SL/Core/Common/Base.h"

namespace sl::ui {

	enum class MenuItemType
	{
		Action,
		Switch,
		Selectable,
		Separator
	};

	struct MenuItem
	{
		MenuItemType type = MenuItemType::Action;
		std::string_view label;
		std::string_view shortcut = "";
		Action<> action;
		bool* display = nullptr;
		bool selected = false;

		MenuItem( MenuItemType itemType )
			: type( itemType )
		{}

		MenuItem( MenuItemType itemType, std::string_view heading, std::string_view key, Action<>&& delegate )
			: type( itemType ), label( heading ), shortcut( key ), action( std::move( delegate ) )
		{}

		MenuItem( MenuItemType itemType, std::string_view heading, std::string_view key, bool& show )
			: type( itemType ), label( heading ), shortcut( key ), display( &show )
		{}

		MenuItem( MenuItemType itemType, std::string_view heading, std::string_view key, bool is_selected, Action<>&& delegate )
			: type( itemType ), label( heading ), shortcut( key ), action( std::move( delegate ) ), selected( is_selected )
		{}
	};

	struct MenuHeading
	{
		std::string_view label;
		std::vector< MenuItem > menu;

		MenuHeading( std::string_view text )
			: label( text )
		{}
	};

	class MenuBar
	{
	public:
		~MenuBar();

	public:
		MenuBar& AddHeading( std::string_view heading );
		MenuBar& AddMenuItemAction( std::string_view label, Action<>&& action );
		MenuBar& AddMenuItemSwitch( std::string_view label, bool& show );
		MenuBar& AddMenuItemSelectable( std::string_view label, bool selected, Action<>&& action );
		MenuBar& AddMenuItemAction( std::string_view label, std::string_view shortcut, Action<>&& action );
		MenuBar& AddMenuItemSwitch( std::string_view label, std::string_view shortcut, bool& show );
		MenuBar& AddMenuItemSelectable( std::string_view label, std::string_view shortcut, bool selected, Action<>&& action );
		MenuBar& AddSeparator();

	private:
		std::vector< MenuHeading > mMenuItems;
	};

	class MainMenuBar
	{
	public:
		~MainMenuBar();

	public:
		MainMenuBar& AddHeading( std::string_view heading );
		MainMenuBar& AddMenuItemAction( std::string_view label, Action<>&& action );
		MainMenuBar& AddMenuItemSwitch( std::string_view label, bool& show );
		MainMenuBar& AddMenuItemSelectable( std::string_view label, bool selected, Action<>&& action );
		MainMenuBar& AddMenuItemAction( std::string_view label, std::string_view shortcut, Action<>&& action );
		MainMenuBar& AddMenuItemSwitch( std::string_view label, std::string_view shortcut, bool& show );
		MainMenuBar& AddMenuItemSelectable( std::string_view label, std::string_view shortcut, bool selected, Action<>&& action );
		MainMenuBar& AddSeparator();

	private:
		std::vector< MenuHeading > mMenuItems;
	};

} // namespace sl::ui
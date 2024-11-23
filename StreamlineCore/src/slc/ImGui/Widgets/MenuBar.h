#pragma once

#include "slc/Common/Base.h"

namespace slc::UI {

	enum class MenuItemType
	{
		Action, Switch, Separator
	};

	struct MenuItem
	{
		MenuItemType type = MenuItemType::Action;
		std::string_view label;
		std::string_view shortcut = "";
		Action<> action;
		bool* display = nullptr;

		MenuItem(MenuItemType itemType) : type(itemType) {}
		MenuItem(MenuItemType itemType, std::string_view heading, std::string_view key, Action<>&& delegate)
			: type(itemType), label(heading), shortcut(key), action(std::move(delegate)) {}
		MenuItem(MenuItemType itemType, std::string_view heading, std::string_view key, bool& show)
			: type(itemType), label(heading), shortcut(key), display(&show) {}
	};

	struct MenuHeading
	{
		std::string_view label;
		std::vector<MenuItem> menu;

		MenuHeading(std::string_view text)
			: label(text) {}
	};

	class MenuBar
	{
	public:
		~MenuBar();

	public:
		void AddHeading(std::string_view heading);
		void AddMenuItemAction(std::string_view label, std::string_view shortcut, Action<>&& action);
		void AddMenuItemSwitch(std::string_view label, std::string_view shortcut, bool& show);
		void AddSeparator();

	private:
		std::vector<MenuHeading> mMenuItems;
	};
}
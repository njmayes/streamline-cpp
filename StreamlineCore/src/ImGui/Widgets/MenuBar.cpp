#include "MenuBar.h"

#include "imgui.h"

namespace slc::UI {

	MenuBar::~MenuBar()
	{
		if (mMenuItems.empty())
			return;

		if (!ImGui::BeginMenuBar())
			return;

		auto openMenus = mMenuItems.Where([](const auto& heading) { return ImGui::BeginMenu(heading.label.data()); });

		for (const MenuHeading& heading : openMenus)
		{
			for (const MenuItem& item : heading.menu)
			{
				switch (item.type)
				{
				case MenuItemType::Action:
					if (ImGui::MenuItem(item.label.data(), item.shortcut.data()))
						item.action();
					break;

				case MenuItemType::Switch:
					ImGui::MenuItem(item.label.data(), item.shortcut.data(), item.display);
					break;

				case MenuItemType::Separator:
					ImGui::Separator();
					break;
				}
			}

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	void MenuBar::AddHeading(std::string_view heading)
	{
		mMenuItems.emplace_back(heading);
	}

	void MenuBar::AddMenuItemAction(std::string_view label, std::string_view shortcut, Action<>&& action)
	{
		MenuHeading& lastMenu = mMenuItems.back();
		lastMenu.menu.emplace_back(MenuItemType::Action, label, shortcut, std::move(action));
	}

	void MenuBar::AddMenuItemSwitch(std::string_view label, std::string_view shortcut, bool& show)
	{
		MenuHeading& lastMenu = mMenuItems.back();
		lastMenu.menu.emplace_back(MenuItemType::Switch, label, shortcut, show);
	}

	void MenuBar::AddSeparator()
	{
		MenuHeading& lastMenu = mMenuItems.back();
		lastMenu.menu.emplace_back(MenuItemType::Separator);
	}
}
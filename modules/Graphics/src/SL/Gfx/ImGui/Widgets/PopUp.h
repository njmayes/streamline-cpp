#pragma once

#include "SL/Core/Common/Base.h"

namespace sl::ui {

	struct PopUpItem
	{
		std::string_view label;
		Action<> action;

		PopUpItem(std::string_view heading, Action<>&& delegate)
			: label(heading), action(std::move(delegate)) {}
	};

	class PopUp
	{
	public:
		PopUp() = default;
		PopUp(std::string_view strID) : mStrID(strID) {}
		~PopUp();

		PopUp& AddPopUpItem(std::string_view label, Action<>&& action);

	private:
		std::string_view mStrID;
		std::vector<PopUpItem> mPopUpItems;
	};

	class PopUpContext
	{
	public:
		~PopUpContext();

		PopUpContext& AddPopUpItem( std::string_view label, Action<>&& action );

	private:
		std::vector<PopUpItem> mPopUpItems;
	};
}
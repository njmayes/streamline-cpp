#pragma once

#include "Common/Base.h"

namespace slc::UI {

	struct PopUpItem
	{
		std::string_view label;
		Action<> action;

		PopUpItem(std::string_view heading, Action<> delegate)
			: label(heading), action(delegate) {}
	};

	class PopUp
	{
	public:
		PopUp(std::string_view strID) : mStrID(strID) {}
		~PopUp();

		void addPopUpItem(std::string_view label, Action<> action);

	private:
		std::string_view mStrID;
		std::vector<PopUpItem> mPopUpItems;
	};

	class PopUpContext
	{
	public:
		~PopUpContext();

		void addPopUpItem(std::string_view label, Action<> action);

	private:
		std::vector<PopUpItem> mPopUpItems;
	};
}
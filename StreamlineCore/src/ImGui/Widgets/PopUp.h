#pragma once

#include "Common/Base.h"
#include "Collections/Vector.h"

namespace slc::UI {

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

		void AddPopUpItem(std::string_view label, Action<>&& action);

	private:
		std::string_view mStrID;
		Vector<PopUpItem> mPopUpItems;
	};

	class PopUpContext
	{
	public:
		~PopUpContext();

		void AddPopUpItem(std::string_view label, Action<>&& action);

	private:
		Vector<PopUpItem> mPopUpItems;
	};
}
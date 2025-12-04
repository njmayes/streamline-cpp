#include "IModal.h"

#include "SL/Core/Common/Application.h"
#include "SL/Core/ImGui/Widgets.h"

namespace slc {

	IModal::IModal()
	{
		Application::BlockEsc();
	}

	IModal::~IModal()
	{
		Application::BlockEsc( false );
	}

} // namespace slc
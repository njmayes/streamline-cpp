#include "IModal.h"

#include "SL/Gfx/ImGui/Widgets.h"
#include "SL/Core/Common/Application.h"

namespace sl {

	IModal::IModal()
	{
		Application::BlockEsc();
	}

	IModal::~IModal()
	{
		Application::BlockEsc( false );
	}

} // namespace sl
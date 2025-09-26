#include "IModal.h"

#include "slc/Common/Application.h"
#include "slc/ImGui/Widgets.h"

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
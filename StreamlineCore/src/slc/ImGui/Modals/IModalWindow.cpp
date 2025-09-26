#include "IModalWindow.h"

#include "slc/Common/Application.h"
#include "slc/ImGui/Widgets.h"

namespace slc {

	IModalWindow::IModalWindow()
	{
		Application::BlockEsc();
	}

	IModalWindow::~IModalWindow()
	{
		Application::BlockEsc( false );
	}

	void WarningModal::OnOverlayRender()
	{
		Widgets::Label( mMessage );
	}

} // namespace slc
#include "GuiApplication.h"

#include "SL/Gfx/Render/Renderer.h"

#include <GLFW/glfw3.h>

namespace sl {

	GuiApplication::GuiApplication( Ref< GuiApplicationSpecification > spec )
		: Application( spec )
	{
		mWindow = Application::CreateEventDevice< Window >( spec->window_props );
		mImGuiController = Application::CreateEventDevice< ImGuiController >( mWindow->GetNativeWindow() );

		RegisterSystem< Renderer >();
	}

	void GuiApplication::OnUpdate( Timestep ts )
	{
		if ( mState.minimised )
			return;

		auto const& layers = GetLayerStack();

		// Regular layer rendering
		for ( auto layer : layers )
			layer->OnRender();

		// Begin ImGui rendering
		mImGuiController->StartFrame();

		// Render each ImGui controls in each layer
		for ( auto layer : layers )
			layer->OnOverlayRender();

		mPanelManager.Render();
		mModalManager.Render();

		// End ImGui rendering
		mImGuiController->EndFrame();

		// Swap buffers
		mWindow->OnUpdate();
	}

	bool GuiApplication::OnEvent( Event& e )
	{
		return e.Dispatch(
			BindDispatch( this, &GuiApplication::OnWindowClose ),
			BindDispatch( this, &GuiApplication::OnWindowResize )
		);
	}

	bool GuiApplication::OnWindowClose( WindowCloseEvent& e )
	{
		Application::Close();
		return true;
	}

	bool GuiApplication::OnWindowResize( WindowResizeEvent& e )
	{
		if ( e.width == 0 || e.height == 0 )
		{
			mState.minimised = true;
			return false;
		}

		mState.minimised = false;
		Renderer::SetViewport( e.width, e.height );
		return false;
	}
} // namespace sl
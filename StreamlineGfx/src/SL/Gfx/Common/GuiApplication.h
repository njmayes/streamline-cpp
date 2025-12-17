#pragma once

#include "SL/Gfx/Input/Window.h"
#include "SL/Gfx/ImGui/Controller.h"

#include "SL/Core/Common/Application.h"

struct GLFWwindow;

namespace sl {

	struct GuiApplicationSpecification : public ApplicationSpecification
	{
		Resolution resolution = { 1600, 900 };
	};

	class GuiApplication : public Application
	{
	public:
		GuiApplication( Ref< GuiApplicationSpecification > spec );
		virtual ~GuiApplication() = default;

		void OnEvent( Event& e ) override;
		void OnUpdate( Timestep ts ) override;
		void OnRender() override;

		SLC_LISTENING_EVENTS( WindowClose, WindowResize )

	private:
		bool OnWindowClose( WindowCloseEvent& e );
		bool OnWindowResize( WindowResizeEvent& e );

	public:
		Window& GetWindow()
		{
			return *mWindow;
		}

		static GLFWwindow* GetNativeWindow()
		{
			auto instance = Get< GuiApplication >();
			return instance->mWindow->GetNativeWindow();
		}

		static float GetWindowWidth()
		{
			auto instance = Get< GuiApplication >();
			return static_cast< float >( instance->mWindow->GetWidth() );
		}
		static float GetWindowHeight()
		{
			auto instance = Get< GuiApplication >();
			return static_cast< float >( instance->mWindow->GetHeight() );
		}

		template < IsModal T, typename... Args >
		static void OpenModal( ModalConstructionData const& init_data, Args&&... args )
		{
			auto instance = Get< GuiApplication >();

			if ( !instance )
				return;

			if ( !instance->mImGuiController )
				return;

			instance->mImGuiController->OpenModal< T >( init_data, std::forward< Args >( args )... );
		}

		template < IsPanel T, typename... Args >
		static void OpenPanel( PanelConstructionData const& init_data, Args&&... args )
		{
			auto instance = Get< GuiApplication >();

			if ( !instance )
				return;

			if ( !instance->mImGuiController )
				return;

			instance->mImGuiController->OpenPanel< T >( init_data, std::forward< Args >( args )... );
		}

	private:
		Box< Window > mWindow;
		Ref< ImGuiController > mImGuiController;
	};
}
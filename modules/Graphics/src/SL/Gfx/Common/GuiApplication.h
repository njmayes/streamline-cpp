#pragma once

#include "SL/Gfx/Input/Window.h"

#include "SL/Gfx/ImGui/Controller.h"
#include "SL/Gfx/ImGui/Modals/ModalManager.h"
#include "SL/Gfx/ImGui/Panels/PanelManager.h"

#include "SL/Core/Common/Application.h"

struct GLFWwindow;

namespace sl {

	struct GuiApplicationSpecification : public ApplicationSpecification
	{
		WindowProperties window_props{};
	};

	class GuiApplication : public Application
	{
	public:
		GuiApplication( Ref< GuiApplicationSpecification > spec );
		virtual ~GuiApplication() = default;

		void OnEvent( Event& e ) override;
		void OnUpdate( Timestep ts ) override;
		void OnRender() override;

		SL_LISTENING_EVENTS( WindowCloseEvent, WindowResizeEvent )

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

			if ( !instance )
				throw std::runtime_error( "No gui application instance" );

			return instance->mWindow->GetNativeWindow();
		}

		static float GetWindowWidth()
		{
			auto instance = Get< GuiApplication >();

			if ( !instance )
				throw std::runtime_error( "No gui application instance" );

			return static_cast< float >( instance->mWindow->GetWidth() );
		}
		static float GetWindowHeight()
		{
			auto instance = Get< GuiApplication >();

			if ( !instance )
				throw std::runtime_error( "No gui application instance" );

			return static_cast< float >( instance->mWindow->GetHeight() );
		}

		template < IsModal T, typename... Args >
		static Ref< T > OpenModal( ModalConstructionData const& init_data, Args&&... args )
		{
			auto instance = Get< GuiApplication >();
			if ( !instance )
				throw std::runtime_error( "No gui application instance" );

			return instance->mModalManager.Open< T >( init_data, std::forward< Args >( args )... );
		}

		static void CloseModal( std::string_view heading )
		{
			auto instance = Get< GuiApplication >();
			if ( !instance )
				throw std::runtime_error( "No gui application instance" );

			return instance->mModalManager.Close( heading );
		}

		template < IsPanel T, typename... Args >
		static Ref< T > OpenPanel( PanelConstructionData const& init_data, Args&&... args )
		{
			auto instance = Get< GuiApplication >();
			if ( !instance )
				throw std::runtime_error( "No gui application instance" );

			return instance->mPanelManager.Open< T >( init_data, std::forward< Args >( args )... );
		}

	private:
		Box< Window > mWindow;
		Ref< ImGuiController > mImGuiController;

		ModalManager mModalManager{};
		PanelManager mPanelManager{};
	};
} // namespace sl
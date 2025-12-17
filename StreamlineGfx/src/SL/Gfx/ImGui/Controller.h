#pragma once

#include "Modals/ModalManager.h"
#include "Panels/PanelManager.h"

#include "SL/Core/Events/IEventListener.h"

struct GLFWwindow;

namespace sl {

	class ImGuiController : public IEventListener
	{
	public:
		ImGuiController( GLFWwindow* window );
		~ImGuiController();

		ImGuiController( const ImGuiController& ) = delete;
		ImGuiController& operator=( const ImGuiController& ) = delete;

		ImGuiController( ImGuiController&& ) = delete;
		ImGuiController& operator=( ImGuiController&& ) = delete;

		void StartFrame();
		void EndFrame();

		template < IsModal T, typename... Args >
		void OpenModal( ModalConstructionData const& init_data, Args&&... args )
		{
			mModalManager.Open< T >( init_data, std::forward< Args >( args )... );
		}

		template < IsPanel T, typename... Args >
		Ref< T >  OpenPanel( PanelConstructionData const& init_data, Args&&... args )
		{
			mPanelManager.Open< T >( init_data, std::forward< Args >( args )... );
		}

		template < typename... Args >
		static Box< ImGuiController > Create( Args&&... args )
		{
			return MakeBox< ImGuiController >( std::forward< Args >( args )... );
		}


	public:
		void OnEvent( Event& e );
		SLC_LISTENING_EVENTS( EVENT_CATEGORY_MOUSE, EVENT_CATEGORY_KEY )

	private:
		void SetDarkThemeColours();

	private:
		bool mBlockEvents = false;
		ModalManager mModalManager{};
		PanelManager mPanelManager{};
	};
} // namespace sl
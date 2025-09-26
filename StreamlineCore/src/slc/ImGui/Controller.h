#pragma once

#include "Modals/ModalManager.h"

#include "slc/Events/IEventListener.h"

struct GLFWwindow;

namespace slc {

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

		template < IsEditorModal T, typename... Args >
		void OpenModal( std::string_view title, ModalButtons type, Args&&... args )
		{
			mModalManager.Open< T >( title, type, std::forward< Args >( args )... );
		}

		template < typename... Args >
		static Box< ImGuiController > Create( Args&&... args )
		{
			return MakeBox< ImGuiController >( std::forward< Args >( args )... );
		}


	public:
		void OnEvent( Event& e );
		LISTENING_EVENTS( EVENT_CATEGORY_MOUSE, EVENT_CATEGORY_KEY )

		void BlockEvents( bool block )
		{
			mBlockEvents = block;
		}

	private:
		void SetDarkThemeColours();

	private:
		bool mBlockEvents = false;
		ModalManager mModalManager{};
	};
} // namespace slc
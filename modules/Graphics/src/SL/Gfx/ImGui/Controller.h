#pragma once

#include "SL/Core/Common/Application.h"

struct GLFWwindow;

namespace sl {

	class ImGuiController : public ApplicationEventListener
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

		template < typename... Args >
		static Box< ImGuiController > Create( Args&&... args )
		{
			return MakeBox< ImGuiController >( std::forward< Args >( args )... );
		}

	public:
		void OnEvent( Event& e );

		SL_LISTENING_EVENTS( MouseEvents, KeyEvents )

	private:
		bool OnMouseEvent( MouseEvent& e );
		bool OnKeyEvent( KeyEvent& e );

		void SetDarkThemeColours();
	};
} // namespace sl
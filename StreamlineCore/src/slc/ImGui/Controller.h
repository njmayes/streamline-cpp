#pragma once

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

		void StartFrame() const;
		void EndFrame() const;

		template < typename... Args >
		static Unique< ImGuiController > Create( Args&&... args )
		{
			return MakeUnique< ImGuiController >( std::forward< Args >( args )... );
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
	};
} // namespace slc
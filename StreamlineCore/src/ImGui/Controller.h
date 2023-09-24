#pragma once

#include "Events/IEventListener.h"

namespace slc {

	class ImGuiController : public IEventListener
	{
	public:
		ImGuiController();
		~ImGuiController();

		ImGuiController(const ImGuiController&) = delete;
		ImGuiController& operator=(const ImGuiController&) = delete;

		ImGuiController(ImGuiController&&) = delete;
		ImGuiController& operator=(ImGuiController&&) = delete;

		void StartFrame() const;
		void EndFrame() const;

    public:
		void OnEvent(Event& e) {}
		LISTENING_EVENTS(EVENT_CATEGORY_MOUSE, EVENT_CATEGORY_KEY)

		void BlockEvents(bool block) { mBlockEvents = block; }

	private:
		void SetDarkThemeColours();

	private:
		bool mBlockEvents = false;
	};
}
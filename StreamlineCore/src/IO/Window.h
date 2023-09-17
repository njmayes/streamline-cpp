#pragma once

#include "Common/Base.h"

struct GLFWwindow;

namespace slc {

    class Event; 

	struct WindowProperties {
		std::string title;
		unsigned width;
		unsigned height;
		bool fullscreen;

		WindowProperties(std::string_view t = "Labyrinth Engine",
			unsigned w = 1600,
			unsigned h = 900,
			bool f = false)
			: title(t), width(w), height(h), fullscreen(f) {}
	};

	class Window
	{
	public:
		using EventCallbackFunc = std::function<void(Event&)>;

		Window(const WindowProperties& props);
		virtual ~Window();

		void onUpdate();

		unsigned getWidth() const { return mData.width; }
		unsigned getHeight() const { return mData.height; }

		void setTitle(const std::string& title);

		//Attributes
		void setVSync(bool enabled);
		bool isVSync() const;

		GLFWwindow* getNativeWindow() const { return mWindow; }

		static Impl<Window> Create(const WindowProperties& props = WindowProperties());

	private:
		void Init(const WindowProperties& props);
		void Shutdown();

	private:
		GLFWwindow* mWindow;

		struct WindowData
		{
			std::string title;
			unsigned width, height;
			bool vSync;
		};

		WindowData mData;
	};

}
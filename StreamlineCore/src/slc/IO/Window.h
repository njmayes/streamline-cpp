#pragma once

#include "slc/Common/Base.h"
#include "slc/Types/Math.h"

struct GLFWwindow;

namespace slc {

    class Event; 

	struct Resolution
	{
		unsigned width, height;

		std::string ToString() const { return std::format("{}x{}", width, height); }
	};

	struct WindowProperties {
		std::string title;
		unsigned width;
		unsigned height;
		bool fullscreen;

		WindowProperties(std::string_view t = "Labyrinth Engine",
			const Resolution& r = { 1600, 900 },
			bool f = false)
			: title(t), width(r.width), height(r.height), fullscreen(f) {}
	};

	class Window
	{
	public:
		Window(const WindowProperties& props);
		virtual ~Window();

		void OnUpdate();

		unsigned GetWidth() const { return mData.width; }
		unsigned GetHeight() const { return mData.height; }

		Vector2f GetSize() const { return { mData.width, mData.height }; }

		void SetTitle(std::string_view title);

		//Attributes
		void SetVSync(bool enabled);
		bool IsVSync() const;

		GLFWwindow* GetNativeWindow() const { return mWindow; }

		static Unique<Window> Create(const WindowProperties& props = WindowProperties());

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
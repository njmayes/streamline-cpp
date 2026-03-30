#pragma once

#include "SL/Core/Common/Base.h"
#include "SL/Core/Types/Math.h"

struct GLFWwindow;

namespace sl {

	class Event;

	struct Resolution
	{
		unsigned width, height;

		std::string ToString() const
		{
			return std::format( "{}x{}", width, height );
		}
	};

	enum class WindowMode
	{
		Windowed,
		BorderlessFullscreen,
		ExclusiveFullscreen
	};

	struct WindowProperties
	{
		std::string title;
		Resolution resolution{ 1280, 720 };

		WindowMode mode = WindowMode::Windowed;
		bool resizable = true;
		bool maximised = false;
		bool centered = true;
		bool visible = true;
		bool vsync = true;

		std::optional< std::uint32_t > monitor_index;
	};

	class Window
	{
	public:
		Window( const WindowProperties& props = {} );
		virtual ~Window();

		void OnUpdate();

		unsigned GetWidth() const
		{
			return mData.width;
		}
		unsigned GetHeight() const
		{
			return mData.height;
		}

		Vec2f GetSize() const
		{
			return { mData.width, mData.height };
		}

		void SetTitle( std::string_view title );

		// Attributes
		void SetVSync( bool enabled );
		bool IsVSync() const;

		GLFWwindow* GetNativeWindow() const
		{
			return mWindow;
		}

	private:
		void Init( const WindowProperties& props );
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

} // namespace sl
#pragma once

#include "SL/Core/Common/Application.h"
#include "SL/Core/Types/Math.h"

struct GLFWwindow;

namespace sl {

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

	class Window : public ApplicationEventEmitter
	{
	public:
		Window( const WindowProperties& props = {} );
		virtual ~Window();

		void OnUpdate();
		void PollEvents() override;

		unsigned GetWidth() const
		{
			return mWidth;
		}
		unsigned GetHeight() const
		{
			return mHeight;
		}

		Vec2f GetSize() const
		{
			return { mWidth, mHeight };
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

		unsigned mWidth, mHeight;
		std::string mTitle;
		bool mVSync;
	};

} // namespace sl
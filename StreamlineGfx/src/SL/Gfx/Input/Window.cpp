#include "Window.h"

#include "SL/Core/Common/Application.h"
#include "SL/Core/Logging/Log.h"

#include <glad/gl.h>
#include "GLFW/glfw3.h"

namespace sl {

	namespace {

		static uint8_t sGLFWWindowCount = 0;

		static void GLFWErrorCallback( int error, const char* description )
		{
			log::Error( "GLFW Error ({0}): {1}", error, description );
		}

		struct MonitorSelection
		{
			GLFWmonitor* monitor{};
			const GLFWvidmode* mode{};
			std::uint32_t index{};
		};

		MonitorSelection SelectMonitor( std::optional< std::uint32_t > monitor_index )
		{
			int monitor_count = 0;
			GLFWmonitor** monitors = glfwGetMonitors( &monitor_count );

			SL_ASSERT( monitors && monitor_count > 0, "No GLFW monitors were found!" );

			std::uint32_t index = monitor_index.value_or( 0u );
			if ( index >= static_cast< std::uint32_t >( monitor_count ) )
			{
				log::Warn(
					"Requested monitor index {} is out of range, falling back to primary monitor",
					index
				);
				index = 0;
			}

			GLFWmonitor* monitor = monitors[ index ];
			if ( !monitor )
				monitor = glfwGetPrimaryMonitor();

			SL_ASSERT( monitor, "Could not resolve target monitor!" );

			const GLFWvidmode* mode = glfwGetVideoMode( monitor );
			SL_ASSERT( mode, "Could not query monitor video mode!" );

			return {
				.monitor = monitor,
				.mode = mode,
				.index = index
			};
		}

		void CenterWindowOnMonitor( GLFWwindow* window, GLFWmonitor* monitor, int width, int height )
		{
			int monitor_x = 0;
			int monitor_y = 0;
			glfwGetMonitorPos( monitor, &monitor_x, &monitor_y );

			const GLFWvidmode* mode = glfwGetVideoMode( monitor );
			if ( !mode )
				return;

			const int x = monitor_x + ( mode->width - width ) / 2;
			const int y = monitor_y + ( mode->height - height ) / 2;

			glfwSetWindowPos( window, x, y );
		}

	} // namespace

	Window::Window( const WindowProperties& props )
	{
		Init( props );
	}

	Window::~Window()
	{
		Shutdown();
	}

	void Window::OnUpdate()
	{
		glfwPollEvents();
		glfwSwapBuffers( mWindow );
	}

	void Window::SetTitle( std::string_view title )
	{
		mData.title = title;
		glfwSetWindowTitle( mWindow, mData.title.c_str() );
	}

	void Window::SetVSync( bool enabled )
	{
		glfwSwapInterval( enabled ? 1 : 0 );
		mData.vSync = enabled;
	}

	bool Window::IsVSync() const
	{
		return mData.vSync;
	}

	void Window::Init( const WindowProperties& props )
	{
		mData.title = props.title.empty() ? "Streamline" : props.title;
		mData.width = props.resolution.width;
		mData.height = props.resolution.height;
		mData.vSync = props.vsync;

		if ( sGLFWWindowCount == 0 )
		{
			const int success = glfwInit();
			SL_ASSERT( success, "Could not initialize GLFW!" );
			glfwSetErrorCallback( GLFWErrorCallback );
		}

		const auto monitor_selection = SelectMonitor( props.monitor_index );
		GLFWmonitor* target_monitor = monitor_selection.monitor;
		const GLFWvidmode* target_mode = monitor_selection.mode;

		glfwDefaultWindowHints();
#if defined( _DEBUG )
		glfwWindowHint( GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE );
#endif
		glfwWindowHint( GLFW_VISIBLE, props.visible ? GLFW_TRUE : GLFW_FALSE );
		glfwWindowHint( GLFW_RESIZABLE, props.resizable ? GLFW_TRUE : GLFW_FALSE );
		glfwWindowHint( GLFW_DECORATED, GLFW_TRUE );
		glfwWindowHint( GLFW_FOCUSED, GLFW_TRUE );

		GLFWmonitor* create_monitor = nullptr;

		switch ( props.mode )
		{
			case WindowMode::Windowed:
			{
				glfwWindowHint( GLFW_MAXIMIZED, props.maximised ? GLFW_TRUE : GLFW_FALSE );
				break;
			}

			case WindowMode::BorderlessFullscreen:
			{
				mData.width = static_cast< unsigned >( target_mode->width );
				mData.height = static_cast< unsigned >( target_mode->height );

				glfwWindowHint( GLFW_DECORATED, GLFW_FALSE );
				glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );
				glfwWindowHint( GLFW_RED_BITS, target_mode->redBits );
				glfwWindowHint( GLFW_GREEN_BITS, target_mode->greenBits );
				glfwWindowHint( GLFW_BLUE_BITS, target_mode->blueBits );
				glfwWindowHint( GLFW_REFRESH_RATE, target_mode->refreshRate );
				break;
			}

			case WindowMode::ExclusiveFullscreen:
			{
				if ( props.resolution.width == 0 || props.resolution.height == 0 )
				{
					mData.width = static_cast< unsigned >( target_mode->width );
					mData.height = static_cast< unsigned >( target_mode->height );
				}

				glfwWindowHint( GLFW_RED_BITS, target_mode->redBits );
				glfwWindowHint( GLFW_GREEN_BITS, target_mode->greenBits );
				glfwWindowHint( GLFW_BLUE_BITS, target_mode->blueBits );
				glfwWindowHint( GLFW_REFRESH_RATE, target_mode->refreshRate );

				create_monitor = target_monitor;
				break;
			}
		}

		log::Trace(
			"Creating window {} ({}, {}) on monitor {} in mode {}",
			mData.title,
			mData.width,
			mData.height,
			monitor_selection.index,
			static_cast< int >( props.mode )
		);

		mWindow = glfwCreateWindow(
			static_cast< int >( mData.width ),
			static_cast< int >( mData.height ),
			mData.title.c_str(),
			create_monitor,
			nullptr
		);
		++sGLFWWindowCount;

		SL_ASSERT( mWindow, "Could not create GLFW window!" );

		if ( props.mode == WindowMode::BorderlessFullscreen )
		{
			int monitor_x = 0;
			int monitor_y = 0;
			glfwGetMonitorPos( target_monitor, &monitor_x, &monitor_y );
			glfwSetWindowPos( mWindow, monitor_x, monitor_y );
		}
		else if ( props.mode == WindowMode::Windowed && props.centered && !props.maximised )
		{
			CenterWindowOnMonitor(
				mWindow,
				target_monitor,
				static_cast< int >( mData.width ),
				static_cast< int >( mData.height )
			);
		}

		glfwMakeContextCurrent( mWindow );

		const int status = gladLoadGL( glfwGetProcAddress );
		SL_ASSERT( status, "Failed to initialize Glad!" );

		log::Info( "OpenGL Info:" );
		log::Info( "\tVendor: {0}", reinterpret_cast< const char* >( glGetString( GL_VENDOR ) ) );
		log::Info( "\tRenderer: {0}", reinterpret_cast< const char* >( glGetString( GL_RENDERER ) ) );
		log::Info( "\tVersion: {0}", reinterpret_cast< const char* >( glGetString( GL_VERSION ) ) );

		glfwSetWindowUserPointer( mWindow, &mData );
		SetVSync( props.vsync );

		glfwSetWindowSizeCallback( mWindow, []( GLFWwindow* window, int width, int height ) {
			WindowData& data = *( WindowData* )glfwGetWindowUserPointer( window );
			data.width = static_cast< unsigned >( width );
			data.height = static_cast< unsigned >( height );

			Application::PostEvent< WindowResizeEvent >( width, height );
		} );

		glfwSetWindowCloseCallback( mWindow, []( GLFWwindow* window ) {
			Application::PostEvent< WindowCloseEvent >();
		} );

		glfwSetWindowPosCallback( mWindow, []( GLFWwindow* window, int xpos, int ypos ) {
			Application::PostEvent< WindowMovedEvent >( xpos, ypos );
		} );

		glfwSetKeyCallback( mWindow, []( GLFWwindow* window, int key, int scancode, int action, int mods ) {
			switch ( action )
			{
				case GLFW_PRESS:
				{
					Application::PostEvent< KeyPressedEvent >( ( KeyCode )key, false );
					break;
				}
				case GLFW_RELEASE:
				{
					Application::PostEvent< KeyReleasedEvent >( ( KeyCode )key );
					break;
				}
				case GLFW_REPEAT:
				{
					Application::PostEvent< KeyPressedEvent >( ( KeyCode )key, true );
					break;
				}
			}
		} );

		glfwSetCharCallback( mWindow, []( GLFWwindow* window, unsigned int keycode ) {
			Application::PostEvent< KeyTypedEvent >( keycode );
		} );

		glfwSetMouseButtonCallback( mWindow, []( GLFWwindow* window, int button, int action, int mods ) {
			switch ( action )
			{
				case GLFW_PRESS:
				{
					Application::PostEvent< MouseButtonPressedEvent >( button );
					break;
				}
				case GLFW_RELEASE:
				{
					Application::PostEvent< MouseButtonReleasedEvent >( button );
					break;
				}
			}
		} );

		glfwSetScrollCallback( mWindow, []( GLFWwindow* window, double xOffset, double yOffset ) {
			Application::PostEvent< MouseScrolledEvent >( ( float )xOffset, ( float )yOffset );
		} );

		glfwSetCursorPosCallback( mWindow, []( GLFWwindow* window, double xPos, double yPos ) {
			Application::PostEvent< MouseMovedEvent >( ( float )xPos, ( float )yPos );
		} );

		glfwSetWindowFocusCallback( mWindow, []( GLFWwindow* window, int focused ) {
			if ( focused == GLFW_TRUE )
				Application::PostEvent< WindowFocusEvent >();
			else
				Application::PostEvent< WindowFocusLostEvent >();
		} );
	}

	void Window::Shutdown()
	{
		if ( mWindow )
		{
			glfwDestroyWindow( mWindow );
			mWindow = nullptr;
			--sGLFWWindowCount;
		}

		if ( sGLFWWindowCount == 0 )
			glfwTerminate();

		log::Info( "Window shutdown complete" );
	}

} // namespace sl
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

			SL_VERIFY( monitors && monitor_count > 0, "No GLFW monitors were found!" );

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

			SL_VERIFY( monitor, "Could not resolve target monitor!" );

			const GLFWvidmode* mode = glfwGetVideoMode( monitor );
			SL_VERIFY( mode, "Could not query monitor video mode!" );

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

		template < typename TEvent, typename... TArgs >
		void GlfwEmit( GLFWwindow* window, TArgs&&... args )
		{
			static_cast< Window* >( glfwGetWindowUserPointer( window ) )->Emit< TEvent >( std::forward< TArgs >( args )... );
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

	void Window::PollEvents()
	{
		glfwPollEvents();
	}

	void Window::SetTitle( std::string_view title )
	{
		mTitle = title;
		glfwSetWindowTitle( mWindow, mTitle.data() );
	}

	void Window::SetVSync( bool enabled )
	{
		glfwSwapInterval( enabled ? 1 : 0 );
		mVSync = enabled;
	}

	bool Window::IsVSync() const
	{
		return mVSync;
	}

	void Window::Init( const WindowProperties& props )
	{
		mTitle = props.title.empty() ? "Streamline" : props.title;
		mWidth = props.resolution.width;
		mHeight = props.resolution.height;
		mVSync = props.vsync;

		if ( sGLFWWindowCount == 0 )
		{
			const int success = glfwInit();
			SL_VERIFY( success, "Could not initialize GLFW!" );
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
				mWidth = static_cast< unsigned >( target_mode->width );
				mHeight = static_cast< unsigned >( target_mode->height );

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
					mWidth = static_cast< unsigned >( target_mode->width );
					mHeight = static_cast< unsigned >( target_mode->height );
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
			mTitle,
			mWidth,
			mHeight,
			monitor_selection.index,
			static_cast< int >( props.mode )
		);

		mWindow = glfwCreateWindow(
			static_cast< int >( mWidth ),
			static_cast< int >( mHeight ),
			mTitle.data(),
			create_monitor,
			nullptr
		);
		++sGLFWWindowCount;

		SL_VERIFY( mWindow, "Could not create GLFW window!" );

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
				static_cast< int >( mWidth ),
				static_cast< int >( mHeight )
			);
		}

		glfwMakeContextCurrent( mWindow );

		const int status = gladLoadGL( glfwGetProcAddress );
		SL_VERIFY( status, "Failed to initialize Glad!" );

		log::Info( "OpenGL Info:" );
		log::Info( "\tVendor: {0}", reinterpret_cast< const char* >( glGetString( GL_VENDOR ) ) );
		log::Info( "\tRenderer: {0}", reinterpret_cast< const char* >( glGetString( GL_RENDERER ) ) );
		log::Info( "\tVersion: {0}", reinterpret_cast< const char* >( glGetString( GL_VERSION ) ) );

		glfwSetWindowUserPointer( mWindow, this );
		SetVSync( props.vsync );

		glfwSetWindowSizeCallback( mWindow, []( GLFWwindow* window, int width, int height ) {
			Window* self = static_cast< Window* >( glfwGetWindowUserPointer( window ) );
			self->mWidth = static_cast< unsigned >( width );
			self->mHeight = static_cast< unsigned >( height );

			self->Emit< WindowResizeEvent >( width, height );
		} );

		glfwSetWindowCloseCallback( mWindow, []( GLFWwindow* window ) {
			GlfwEmit< WindowCloseEvent >(window);
		} );

		glfwSetWindowPosCallback( mWindow, []( GLFWwindow* window, int xpos, int ypos ) {
			GlfwEmit< WindowMovedEvent >( window, xpos, ypos );
		} );

		glfwSetKeyCallback( mWindow, []( GLFWwindow* window, int key, int scancode, int action, int mods ) {
			switch ( action )
			{
				case GLFW_PRESS:
				{
					GlfwEmit< KeyPressedEvent >( window, ( KeyCode )key, false );
					break;
				}
				case GLFW_RELEASE:
				{
					GlfwEmit< KeyReleasedEvent >( window, ( KeyCode )key );
					break;
				}
				case GLFW_REPEAT:
				{
					GlfwEmit< KeyPressedEvent >( window, ( KeyCode )key, true );
					break;
				}
			}
		} );

		glfwSetCharCallback( mWindow, []( GLFWwindow* window, unsigned int keycode ) {
			GlfwEmit< KeyTypedEvent >( window, keycode );
		} );

		glfwSetMouseButtonCallback( mWindow, []( GLFWwindow* window, int button, int action, int mods ) {
			switch ( action )
			{
				case GLFW_PRESS:
				{
					GlfwEmit< MouseButtonPressedEvent >( window, button );
					break;
				}
				case GLFW_RELEASE:
				{
					GlfwEmit< MouseButtonReleasedEvent >( window, button );
					break;
				}
			}
		} );

		glfwSetScrollCallback( mWindow, []( GLFWwindow* window, double xOffset, double yOffset ) {
			GlfwEmit< MouseScrolledEvent >( window, ( float )xOffset, ( float )yOffset );
		} );

		glfwSetCursorPosCallback( mWindow, []( GLFWwindow* window, double xPos, double yPos ) {
			GlfwEmit< MouseMovedEvent >( window,  ( float )xPos, ( float )yPos );
		} );

		glfwSetWindowFocusCallback( mWindow, []( GLFWwindow* window, int focused ) {
			if ( focused == GLFW_TRUE )
				GlfwEmit< WindowFocusEvent >( window );
			else
				GlfwEmit< WindowFocusLostEvent >( window );
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
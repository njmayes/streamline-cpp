#include "pch.h"
#include "Window.h"

#include <glad/glad.h>
#include "GLFW/glfw3.h"

#include "Events/EventManager.h"

namespace slc {

	static uint8_t sGLFWWindowCount = 0;

	static void GLFWErrorCallback(int error, const char* description)
	{
		LOG("GLFW Error ({0}): {1}", error, description);
	}

	Impl<Window> Window::Create(const WindowProperties& props)
	{
		return MakeImpl<Window>(props);
	}

	Window::Window(const WindowProperties& props)
	{
		Init(props);
	}

	Window::~Window()
	{
		Shutdown();
	}

	void Window::onUpdate()
	{
		glfwPollEvents();
		glfwSwapBuffers(mWindow);
	}

	void Window::setTitle(std::string_view title)
	{
		mData.title = title;
		glfwSetWindowTitle(mWindow, title.data());
	}

	void Window::setVSync(bool enabled)
	{
		if (enabled)
			glfwSwapInterval(1);
		else
			glfwSwapInterval(0);

		mData.vSync = enabled;
	}

	bool Window::isVSync() const
	{
		return mData.vSync;
	}

	void Window::Init(const WindowProperties& props)
	{
		mData.title = props.title;
		mData.width = props.width;
		mData.height = props.height;

		if (sGLFWWindowCount == 0)
		{
			int success = glfwInit();
			ASSERT(success, "Could not initialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);
		}

		if (props.fullscreen)
		{	// Set Window size to be full size of primary monitor and set borderless (prefer this to actual fullscreen)
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			mData.width = mode->width;
			mData.height = mode->height;
			glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		}

		LOG("Creating window {0} ({1}, {2})", mData.title, mData.width, mData.height);

		{
#if defined(_DEBUG)
			glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
			mWindow = glfwCreateWindow((int)mData.width, (int)mData.height, mData.title.c_str(), nullptr, nullptr);
			++sGLFWWindowCount;
		}
		ASSERT(mWindow, "Could not create GLFW window!");

		glfwMakeContextCurrent(mWindow);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		ASSERT(status, "Failed to initialize Glad!");

		LOG("OpenGL Info:");
		LOG("  Vendor: {0}", glGetString(GL_VENDOR));
		LOG("  Renderer: {0}", glGetString(GL_RENDERER));
		LOG("  Version: {0}", glGetString(GL_VERSION));

		ASSERT(GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 5), "Streamline requires at least OpenGL version 4.5!");

		glfwSetWindowUserPointer(mWindow, &mData);
		setVSync(true);

		// Set GLFW callbacks
		glfwSetWindowSizeCallback(mWindow, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.width = width;
			data.height = height;

			EventManager::Post<WindowResizeEvent>(width, height);
		});

		glfwSetWindowCloseCallback(mWindow, [](GLFWwindow* window)
		{
			EventManager::Post<WindowCloseEvent>();
		});

		glfwSetWindowPosCallback(mWindow, [](GLFWwindow* window, int xpos, int ypos)
		{
			EventManager::Post<WindowMovedEvent>(xpos, ypos);
		});

		glfwSetKeyCallback(mWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			switch (action)
			{
			case GLFW_PRESS:
			{
				EventManager::Post<KeyPressedEvent>((KeyCode)key, false);
				break;
			}
			case GLFW_RELEASE:
			{
				EventManager::Post<KeyReleasedEvent>((KeyCode)key);
				break;
			}
			case GLFW_REPEAT:
			{
				EventManager::Post<KeyPressedEvent>((KeyCode)key, true);
				break;
			}
			}
		});

		glfwSetCharCallback(mWindow, [](GLFWwindow* window, unsigned int keycode)
		{
			EventManager::Post<KeyTypedEvent>(keycode);
		});

		glfwSetMouseButtonCallback(mWindow, [](GLFWwindow* window, int button, int action, int mods)
		{			switch (action)
			{
			case GLFW_PRESS:
			{
				EventManager::Post<MouseButtonPressedEvent>(button);
				break;
			}
			case GLFW_RELEASE:
			{
				EventManager::Post<MouseButtonReleasedEvent>(button);
				break;
			}
			}
		});

		glfwSetScrollCallback(mWindow, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			EventManager::Post<MouseScrolledEvent>((float)xOffset, (float)yOffset);
		});

		glfwSetCursorPosCallback(mWindow, [](GLFWwindow* window, double xPos, double yPos)
		{
			EventManager::Post<MouseMovedEvent>((float)xPos, (float)yPos);
		});

		glfwSetWindowFocusCallback(mWindow, [](GLFWwindow* window, int focused)
		{
			if (focused == GLFW_TRUE)
				EventManager::Post<WindowFocusEvent>();
			else
				EventManager::Post<WindowFocusLostEvent>();
		});
	}

	void Window::Shutdown()
	{
		glfwDestroyWindow(mWindow);
		--sGLFWWindowCount;

		if (sGLFWWindowCount == 0)
		{
			glfwTerminate();
		}
		LOG("Shutdown complete");
	}
}
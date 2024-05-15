#include "GraphicsContext.h"

#include "slc/Logging/Log.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace slc {

	GraphicsContext::GraphicsContext(GLFWwindow* windowHandle)
		: mWindowHandle(windowHandle)
	{
		ASSERT(windowHandle, "Window handle is null!");
	}

	void GraphicsContext::Init()
	{
		glfwMakeContextCurrent(mWindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		ASSERT(status, "Failed to initialize Glad!");

		const char* vendor = (char*)glGetString(GL_VENDOR);
		const char* renderer = (char*)glGetString(GL_RENDERER);
		const char* version = (char*)glGetString(GL_VERSION);

		Log::Info("OpenGL Info:");
		Log::Info("  Vendor: {0}", vendor);
		Log::Info("  Renderer: {0}", renderer);
		Log::Info("  Version: {0}", version);

		ASSERT(GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 5), "Labyrinth requires at least OpenGL version 4.5!");
	}

	void GraphicsContext::SwapBuffers()
	{
		glfwSwapBuffers(mWindowHandle);
	}

}
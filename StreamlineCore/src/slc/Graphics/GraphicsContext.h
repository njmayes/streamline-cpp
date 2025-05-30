#pragma once

#include <slc/Common/Base.h>

struct GLFWwindow;

namespace slc {

	class GraphicsContext
	{
	public:
		GraphicsContext(GLFWwindow* windowHandle);

		virtual void Init();
		virtual void SwapBuffers();

		static Unique<GraphicsContext> Create(void* window) { return MakeUnique<GraphicsContext>((GLFWwindow*)window); }

	private:
		GLFWwindow* mWindowHandle;
	};

}
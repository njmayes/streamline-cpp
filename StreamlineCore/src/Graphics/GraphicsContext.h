#pragma once

#include <Common/Ref.h>

struct GLFWwindow;

namespace slc {

	class GraphicsContext
	{
	public:
		GraphicsContext(GLFWwindow* windowHandle);

		virtual void Init();
		virtual void SwapBuffers();

		static Impl<GraphicsContext> Create(void* window) { return MakeImpl<GraphicsContext>((GLFWwindow*)window); }

	private:
		GLFWwindow* mWindowHandle;
	};

}
#pragma once

#include <slc/Common/Base.h>

struct GLFWwindow;

namespace slc {

	class GraphicsContext
	{
	public:
		GraphicsContext( GLFWwindow* windowHandle );

		virtual void Init();
		virtual void SwapBuffers();

		static Box< GraphicsContext > Create( void* window )
		{
			return MakeBox< GraphicsContext >( ( GLFWwindow* )window );
		}

	private:
		GLFWwindow* mWindowHandle;
	};

} // namespace slc
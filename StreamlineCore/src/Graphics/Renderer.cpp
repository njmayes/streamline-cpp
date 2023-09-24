#include "pch.h"
#include "Renderer.h"

#include <glad/glad.h>

namespace slc {

	void OpenGLMessageCallback(
		unsigned source,
		unsigned type,
		unsigned id,
		unsigned severity,
		int length,
		const char* message,
		const void* userParam)
	{
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH:         LOG(message); return;
		case GL_DEBUG_SEVERITY_MEDIUM:       LOG(message); return;
		case GL_DEBUG_SEVERITY_LOW:          LOG(message); return;
		case GL_DEBUG_SEVERITY_NOTIFICATION: LOG(message); return;
		}

		ASSERT(false, "Unknown severity level!");
	}

	void Renderer::Init()
	{
#ifdef LAB_DEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(OpenGLMessageCallback, nullptr);

		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
#endif

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		glDepthFunc(GL_LESS);
		glEnable(GL_DEPTH_TEST);

		glEnable(GL_LINE_SMOOTH);

		glClearColor(0, 0, 0, 1);
	}

	void Renderer::SetViewport(unsigned w, unsigned h)
	{
		glViewport(0, 0, w, h);
	}

	void Renderer::Clear()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
}
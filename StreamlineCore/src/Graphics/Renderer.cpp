#include "Renderer.h"

#include <glad/glad.h>

#include "Renderer2D.h"

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

		Renderer2D::Init();
	}

	void Renderer::Shutdown()
	{
		Renderer2D::Shutdown();
	}

	void Renderer::SetViewport(uint32_t w, uint32_t h)
	{
		glViewport(0, 0, w, h);
	}

	void Renderer::SetClearColor(const glm::vec4& colour)
	{
		glClearColor(colour.r, colour.g, colour.b, colour.a);
	}

	void Renderer::Clear()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void Renderer::SetLineWidth(float width)
	{
		glLineWidth(width);
	}

	void Renderer::DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount)
	{
		uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
		vertexArray->Bind();
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
#if LAB_DEBUG
		vertexArray->Unbind();
#endif
	}

	void Renderer::DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount)
	{
		vertexArray->Bind();
		glDrawArrays(GL_LINES, 0, vertexCount);
#if LAB_DEBUG
		vertexArray->Unbind();
#endif
	}

	void Renderer::EnableDepth()
	{
		glEnable(GL_DEPTH_TEST);
	}

	void Renderer::DisableDepth()
	{
		glDisable(GL_DEPTH_TEST);
	}
}
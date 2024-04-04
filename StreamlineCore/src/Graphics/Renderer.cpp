#include "Renderer.h"

#include <glad/glad.h>

namespace slc {

	void Renderer::SetViewport(unsigned w, unsigned h)
	{
		glViewport(0, 0, w, h);
	}
}
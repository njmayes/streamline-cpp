#include "Timestep.h"

#include <GLFW/glfw3.h>

namespace slc {

	float slc::Timestep::Now()
	{
		return ( float )glfwGetTime();
	}
} // namespace slc

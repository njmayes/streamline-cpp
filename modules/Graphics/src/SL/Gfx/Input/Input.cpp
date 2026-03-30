#include "Input.h"

#include <GLFW/glfw3.h>

#include "SL/Gfx/Common/GuiApplication.h"

namespace sl::input {

	bool IsKeyPressed( KeyCode keycode )
	{
		auto* window = static_cast< GLFWwindow* >( Application::Get< GuiApplication >()->GetWindow().GetNativeWindow() );
		auto state = glfwGetKey( window, static_cast< int32_t >( keycode ) );
		return state == GLFW_PRESS;
	}

	bool IsMouseButtonPressed( MouseCode button )
	{
		auto* window = static_cast< GLFWwindow* >( Application::Get< GuiApplication >()->GetWindow().GetNativeWindow() );
		auto state = glfwGetMouseButton( window, static_cast< int32_t >( button ) );
		return state == GLFW_PRESS;
	}

	Vec2f GetMousePosition()
	{
		auto* window = static_cast< GLFWwindow* >( Application::Get< GuiApplication >()->GetWindow().GetNativeWindow() );
		double xpos, ypos;
		glfwGetCursorPos( window, &xpos, &ypos );
		return { static_cast< float >( xpos ), static_cast< float >( ypos ) };
	}

	float GetMouseX()
	{
		return GetMousePosition().x;
	}

	float GetMouseY()
	{
		return GetMousePosition().y;
	}
} // namespace sl::input
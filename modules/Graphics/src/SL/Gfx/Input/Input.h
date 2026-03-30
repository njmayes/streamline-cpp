#pragma once

#include "SL/Core/IO/KeyCodes.h"
#include "SL/Core/IO/MouseCodes.h"

#include "SL/Core/Types/Math.h"

namespace sl::input {

	bool IsKeyPressed( KeyCode keycode );
	bool IsMouseButtonPressed( MouseCode button );

	Vec2f GetMousePosition();
	float GetMouseX();
	float GetMouseY();
} // namespace sl::input
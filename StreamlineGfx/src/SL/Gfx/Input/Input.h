#pragma once

#include "KeyCodes.h"
#include "MouseCodes.h"

#include "SL/Core/Types/Math.h"

namespace slc::input {

	bool IsKeyPressed( KeyCode keycode );
	bool IsMouseButtonPressed( MouseCode button );

	Vec2f GetMousePosition();
	float GetMouseX();
	float GetMouseY();
} // namespace slc::input
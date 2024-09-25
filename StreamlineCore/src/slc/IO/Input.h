#pragma once

#include "KeyCodes.h"
#include "MouseCodes.h"

#include "slc/Types/Math.h"

namespace slc::Input {

	bool IsKeyPressed(KeyCode keycode);
	bool IsMouseButtonPressed(MouseCode button);

	Vector2 GetMousePosition();
	float GetMouseX();
	float GetMouseY();
}
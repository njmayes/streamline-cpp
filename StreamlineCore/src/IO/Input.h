#pragma once

#include "IO/KeyCodes.h"
#include "IO/MouseCodes.h"

#include "Types/Math.h"

namespace slc {

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode keycode);
		static bool IsMouseButtonPressed(MouseCode button);

		static Vector2 GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();
	};

}
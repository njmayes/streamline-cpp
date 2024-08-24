#pragma once

#include "slc/IO/KeyCodes.h"
#include "slc/IO/MouseCodes.h"

#include "slc/Types/Math.h"

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
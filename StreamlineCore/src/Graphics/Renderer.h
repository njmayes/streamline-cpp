#pragma once

#include "Common/Base.h"

namespace slc {

	class Renderer
	{
	public:
		static void Init();

		static void SetViewport(unsigned w, unsigned h);
		static void Clear();
	};

}
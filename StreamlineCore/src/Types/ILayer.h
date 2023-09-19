#pragma once

#include "Common/Base.h"
#include "Events/IEventListener.h"

namespace slc {

	class ILayer : public IEventListener
	{
	public:
		virtual void OnAttach() = 0;
		virtual void OnDetach() = 0;
		virtual void OnRender() = 0;
	};

	template<typename T>
	concept IsLayer = std::is_base_of_v<ILayer, T>;
}
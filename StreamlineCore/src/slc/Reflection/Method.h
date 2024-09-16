#pragma once

#include "Core.h"

namespace slc {

	class Type;

	class Method
	{
	public:
		Method() = default;
		Method(const MethodInfo& info)
			: mMethod(&info)
		{}

		std::string_view GetName() const { return mMethod->name; }

		Type GetReturnType() const;

	private:
		const MethodInfo* mMethod;
	};
}
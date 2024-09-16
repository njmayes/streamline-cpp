#pragma once

#include "Core.h"

namespace slc {

	class Property;
	class Method;

	class Type
	{
	public:
		Type(const TypeInfo* type)
			: mInfo(type) {}

		Property GetProperty(std::string_view name) const;
		std::vector<Property> GetProperties() const;

		Method GetMethod(std::string_view name) const;
		std::vector<Method> GetMethods() const;

	private:
		const TypeInfo* mInfo;
	};
}
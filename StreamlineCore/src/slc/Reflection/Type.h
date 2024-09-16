#pragma once

#include "Core.h"

namespace slc {

	class Property;

	class Type
	{
	public:
		Type(const TypeInfo* type)
			: mInfo(type) {}

		Property GetProperty(std::string_view name) const;
		std::vector<Property> GetProperties() const;

	private:
		const TypeInfo* mInfo;

		friend class Reflection;
	};
}
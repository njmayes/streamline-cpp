#include "Type.h"
#include "Property.h"

namespace slc {

	Property Type::GetProperty(std::string_view name) const
	{
		auto it = std::ranges::find_if(mInfo->properties, [name](const auto& prop) { return prop.name == name; });
		if (it == mInfo->properties.end())
			return {};

		return Property(*it);
	}

	std::vector<Property> Type::GetProperties() const
	{
		return mInfo->properties | 
			std::views::transform([](const auto& property) { return Property(property); }) |
			std::ranges::to<std::vector>();
	}
}
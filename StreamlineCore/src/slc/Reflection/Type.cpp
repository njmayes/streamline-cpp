#include "Type.h"
#include "Property.h"
#include "Method.h"

namespace slc {

	std::vector<Type> Type::GetBaseClasses() const
	{
		return mInfo->base_types |
			std::views::transform([](const auto& type) { return Type(type); }) |
			std::ranges::to<std::vector>();
	}

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

	Method Type::GetMethod(std::string_view name) const
	{
		auto it = std::ranges::find_if(mInfo->methods, [name](const auto& method) { return method.name == name; });
		if (it == mInfo->methods.end())
			return {};

		return Method(*it);
	}

	std::vector<Method> Type::GetMethods() const
	{
		return mInfo->methods |
			std::views::transform([](const auto& method) { return Method(method); }) |
			std::ranges::to<std::vector>();
	}

	bool Type::IsSubclassOf(const Type& other) const
	{
		return std::ranges::any_of(GetBaseClasses(), [other](auto const& type) { return type == other; });
	}
}
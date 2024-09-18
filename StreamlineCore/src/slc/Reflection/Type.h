#pragma once

#include "Property.h"
#include "Method.h"

namespace slc {

	class Type
	{
	public:
		Type(const TypeInfo* type)
			: mInfo(type) {}

		std::string_view GetName() const { return mInfo->name; }

		template<CanReflect T, typename... Args> requires std::constructible_from<T, Args...>
		T Instantiate(Args&&... args)
		{
			auto ctr = FindConstructor<T, Args...>();
			ASSERT(ctr, "Could not find constructor with these args! Make sure you have reflected the constructor");

			std::vector<Instance> args = { MakeInstance(std::forward<Args>(args))... };
			auto instance = ctr->invoker(std::move(args));
			return std::move(instance.data.Get<T>());
		}

		std::vector<Type> GetBaseClasses() const;

		Property GetProperty(std::string_view name) const;
		std::vector<Property> GetProperties() const;

		Method GetMethod(std::string_view name) const;
		std::vector<Method> GetMethods() const;

	private:
		template<typename T, typename... Args>
		const ConstructorInfo* FindConstructor()
		{
			std::vector<const TypeInfo*> types = { Reflection::GetInfo<Args>... };
			auto it = std::ranges::find_if(mInfo->constructors, [&](auto const& ctr) {
				return Reflection::GetInfo<T>() == ctr.parent_type and
					std::ranges::equal(types, ctr.arguments);
			});

			return it != mInfo->constructors.end() ? &(*it) : nullptr;
		}

	private:
		const TypeInfo* mInfo;
	};

	template<CanReflect T>
	Type Reflect()
	{
		return Type(Reflection::GetInfo<T>());
	}
}
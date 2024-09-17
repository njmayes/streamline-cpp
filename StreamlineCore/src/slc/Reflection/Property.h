#pragma once

#include "Core.h"

namespace slc {

	class Type;

	class Property
	{
	public:
		Property() = default;
		Property(const PropertyInfo& info)
			: mProperty(&info)
		{}

		Type GetType() const;
		std::string_view GetName() const { return mProperty->name; }

		template<CanReflect T, CanReflect Obj>
		const T& GetValue(const Obj& obj) const
		{
			using Traits = TypeTraits<T>;
			using ObjTraits = TypeTraits<Obj>;
			ASSERT(Traits::LongName == mProperty->prop_type->name);
			ASSERT(ObjTraits::LongName == mProperty->parent_type->name);

			auto instance = GetValue(obj);
			ASSERT(instance.type == mProperty->prop_type);

			auto ptr = static_cast<const T*>(instance.data);
			return static_cast<const T&>(*ptr);
		}

		template<CanReflect Obj>
		ConstInstance GetValue(const Obj& obj) const
		{
			return mProperty->accessor(MakeInstance(obj));
		}

		template<CanReflect T, CanReflect Obj>
		void SetValue(Obj& obj, const T& value)
		{
			using Traits = TypeTraits<T>;
			ASSERT(Traits::LongName == mProperty->prop_type->name);

			SetValue(obj, MakeInstance(value));
		}

		template<CanReflect Obj>
		void SetValue(Obj& obj, ConstInstance value)
		{
			return mProperty->setter(MakeInstance(obj), std::move(value));
		}

	private:
		const PropertyInfo* mProperty;
	};
}
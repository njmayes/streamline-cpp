#pragma once

#include "Reflection.h"

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

			if (Traits::LongName != mProperty->prop_type->name)
				throw BadReflectionCastException(Traits::LongName, mProperty->prop_type->name);

			if (ObjTraits::LongName != mProperty->parent_type->name)
				throw BadReflectionCastException(ObjTraits::LongName, mProperty->parent_type->name);

			auto instance = GetValue(obj);
			return instance.data.Get<const T&>();
		}

		template<CanReflect Obj>
		Instance GetValue(const Obj& obj) const
		{
			return mProperty->accessor(MakeInstance(obj));
		}

		template<CanReflect T, CanReflect Obj>
		void SetValue(Obj& obj, const T& value)
		{
			using Traits = TypeTraits<T>;

			if (Traits::LongName != mProperty->prop_type->name)
				throw BadReflectionCastException(Traits::LongName, mProperty->prop_type->name);

			SetValue(obj, MakeInstance(value));
		}

		template<CanReflect Obj>
		void SetValue(Obj& obj, Instance value)
		{
			return mProperty->setter(MakeInstance(obj), std::move(value));
		}

	private:
		const PropertyInfo* mProperty;
	};
}
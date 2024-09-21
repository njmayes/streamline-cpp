#pragma once

#include "Reflection.h"

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
		std::vector<Type> GetArgumentTypes() const;

		Instance Invoke(Instance obj, std::vector<Instance>& args) const
		{
			return mMethod->invoker(std::move(obj), args);
		}

		template<CanReflect T, CanReflect Obj, CanReflect... Args>
		std::optional<T> Invoke(Obj& obj, Args&&... args) const
		{
			using ReturnTraits = TypeTraits<T>;
			using ObjTraits = TypeTraits<Obj>;

			if (ReturnTraits::LongName != mMethod->return_type->name)
				throw BadReflectionCastException(ReturnTraits::LongName, mMethod->return_type->name);

			if (ObjTraits::LongName != mMethod->parent_type->name)
				throw BadReflectionCastException(ObjTraits::LongName, mMethod->parent_type->name);

			if (sizeof...(Args) != mMethod->arguments.size())
				throw std::logic_error("Number of arguments provided does not match the number of arguments of the method.");

			auto make_instance_arg = [&]<typename T>(T&& value) -> Instance {
				return MakeInstance(std::forward<T>(value));
			};

			std::vector<Instance> instanced_args = { make_instance_arg(std::forward<Args>(args))... };
			auto result = mMethod->invoker(MakeInstance(obj), std::move(instanced_args));

			if (result.IsVoid())
				return std::nullopt;

			return std::move(result.data.Get<T>());
		}

	private:
		const MethodInfo* mMethod;
	};
}
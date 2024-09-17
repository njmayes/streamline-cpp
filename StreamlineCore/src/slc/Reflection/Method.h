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
		std::vector<Type> GetArgumentTypes() const;

		Instance Invoke(Instance obj, const std::vector<Instance>& args = {}) const
		{
			return mMethod->invoker(std::move(obj), args);
		}

		template<CanReflect T, CanReflect Obj, CanReflect... Args>
		std::optional<T> Invoke(Obj& obj, Args&&... args) const
		{
			using ReturnTraits = TypeTraits<T>;
			using ObjTraits = TypeTraits<Obj>;

			if constexpr (std::derived_from<T, Reflectable<T>>)
				ASSERT(ReturnTraits::LongName == mMethod->return_type->name);

			ASSERT(ObjTraits::LongName == mMethod->parent_type->name);
			ASSERT(sizeof...(Args) == mMethod->arguments.size());

			auto make_instance_arg = [&]<typename T>(T&& value) -> Instance {
				return MakeInstance(value);
			};

			std::vector<Instance> instanced_args{};
			instanced_args.reserve(sizeof...(Args));

			([&]()
			{
				instanced_args.emplace_back(make_instance_arg(std::forward<Args>(args)));
			}(), ...);

			auto result = mMethod->invoker(MakeInstance(obj), instanced_args);

			if (result.IsVoid())
				return std::nullopt;

			return std::move(*(result.As<T>()));
		}

	private:
		const MethodInfo* mMethod;
	};
}
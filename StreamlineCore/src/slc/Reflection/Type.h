#pragma once

#include "Property.h"
#include "Method.h"

#include <iostream>

namespace slc {

	class Type
	{
	public:
		Type(const TypeInfo* type)
			: mInfo(type) {}

		std::string_view GetName() const { return mInfo->name; }

		template<CanReflect T, typename... Args> requires std::constructible_from<T, Args...>
		T Instantiate(Args&&... args) const
		{
			if (mInfo->name != TypeTraits<T>::Name)
				throw BadReflectionCastException(TypeTraits<T>::Name, mInfo->name);

			auto ctr = FindConstructor<Args&&...>();
			if (not ctr)
				throw UnreflectedTargetException<Args...>(mInfo->name);

			std::vector<Instance> instanced_args;
			instanced_args.reserve(sizeof...(Args));

			([&]()
			{
				instanced_args.emplace_back(MakeInstance(std::forward<Args>(args)));
			}(), ...);
			
			auto instance = ctr->invoker(std::move(instanced_args));
			return std::move(instance.data.Get<T>());
		}

		template<typename T, CanReflect Obj, typename... Args>
		T InvokeMember(std::string_view name, Obj&& obj, Args&&... args) const
		{
			return GetMethod(name).Invoke<T>(std::forward<Obj>(obj), std::forward<Args>(args)...);
		}

		std::vector<Type> GetBaseClasses() const;

		Property GetProperty(std::string_view name) const;
		std::vector<Property> GetProperties() const;

		Method GetMethod(std::string_view name) const;
		std::vector<Method> GetMethods() const;

		template<CanReflect T>
		bool IsSubclassOf() const { return IsSubclassOf(Reflection::GetInfo<T>()); }
		bool IsSubclassOf(const Type& other) const;

		auto operator<=>(const Type&) const = default;
		operator bool() const { return mInfo; }

	private:
		template<typename... Args>
		const ConstructorInfo* FindConstructor() const
		{
			using ArgTypes = TypeList<Args...>;

			auto is_covertible = [&]<std::size_t I>(const TypeInfo* ctr_arg) {
				using ArgType = ArgTypes::template Type<I>;
				return IsConvertibleTo<ArgType>(ctr_arg);
			};

			auto match_param = [&] <std::size_t... Is> (const ConstructorInfo& ctr, std::index_sequence<Is...>) {
				return (... and is_covertible.template operator()<Is>(ctr.arguments[Is]));
			};

			auto match_parameters = [&](const ConstructorInfo& ctr) {
				if (ctr.arguments.size() != ArgTypes::Size)
					return false;

				return match_param(ctr, std::make_index_sequence<ArgTypes::Size>());
			};

			auto it = std::ranges::find_if(mInfo->constructors, match_parameters);
			return it != mInfo->constructors.end() ? &(*it) : nullptr;
		}

	private:
		const TypeInfo* mInfo;

		template<typename Arg>
		static bool IsConvertibleTo(const TypeInfo* target)
		{
			using Traits = TypeTraits<Arg>;

			SLC_TODO("Support conversions between types, not just between value categories of same type");
			if (Traits::BaseName != target->base_name)
				return false;

			if constexpr (Traits::IsLValueReference)
			{
				if constexpr (Traits::IsConst)
				{
					//const& parameter can be used for const& argument as well as for value arguments of types with copy constructor
					return (target->rttt.is_const and target->rttt.is_lvalue_reference) or
						(not target->rttt.is_reference and target->rttt.is_copy_constructible);
				}
				else
				{
					//& parameter can be used for & argument as well as for value arguments of types with copy constructor
					return (target->rttt.is_lvalue_reference) or
						(not target->rttt.is_reference and target->rttt.is_copy_constructible);
				}

			}
			else if constexpr (Traits::IsRValueReference)
			{
				//&& parameter can be used for && argument or value arguments with move constructor
				return (target->rttt.is_rvalue_reference) or
					(not target->rttt.is_reference and target->rttt.is_move_constructible);
			}
			else
			{
				//Value paramater can be used for any argument type (except rvalue ref)
				return not target->rttt.is_rvalue_reference;
			}
		}
	};

	template<CanReflect T>
	inline Type Reflect()
	{
		return Type(Reflection::GetInfo<T>());
	}

	inline Type Reflect(std::string_view type_name)
	{
		return Type(Reflection::GetInfo(type_name));
	}
}
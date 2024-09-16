#pragma once

#include "Type.h"
#include "Property.h"
#include "Method.h"

namespace slc {

	using AddTypeJob = std::function<void()>;

	class Reflection
	{
	public:
		static void QueueReflection(AddTypeJob&& job)
		{
			sReflectionState.init_job_queue.push_back(std::move(job));
		}

		static void ProcessQueue() 
		{
			for (const auto& add_type : sReflectionState.init_job_queue)
			{
				add_type();
			}
		}

		template<CanReflect T>
		static Type Get()
		{
			using BaseType = std::remove_cvref_t<std::remove_pointer_t<T>>;
			using Traits = TypeTraits<BaseType>;

			SCONSTEXPR bool IsBuiltInType = std::is_arithmetic_v<BaseType>;
			SCONSTEXPR std::string_view Key = IsBuiltInType ? BuiltInType : Traits::LongName;

			using Traits = TypeTraits<BaseType>;
			if (not sReflectionState.data.contains(Traits::LongName))
				Register<BaseType>();

			return &sReflectionState.data[Key];
		}

		template<CanReflect T, typename... Args> requires std::is_constructible_v<T, Args...>
		static void RegisterConstructor()
		{
			auto typeInfo = GetInfoForAddition<T>();

			using Params = TypeList<Args...>;

			ConstructorInfo ctr;
			ctr.parent_type = typeInfo;

			auto gen_tuple_val = []<std::size_t I>(Instance object) {
				using ArgType = typename Params::template Type<I>;
				return static_cast<ArgType*>(object.data);
			};

			auto gen_tuple = [gen_tuple_val] <std::size_t... Is> (const std::vector<Instance>&args, std::index_sequence<Is...>)-> Params::TupleType {
				return std::make_tuple(gen_tuple_val.template operator()<Is>(args[Is])...);
			};

			ctr.invoker = [gen_tuple](const std::vector<Instance>& args) {
				auto tuple_params = gen_tuple(args, std::make_index_sequence<Params::Size>());

				auto ctr = []<typename... Ts>(Ts&&... args) {
					return new T(std::forward<Ts>(args)...);
				};

				return Instance(
					GetInfo<T>(),
					std::apply(ctr, tuple_params),
					[](void* data) { delete static_cast<T*>(data); }
				);
			};

			typeInfo->constructors.push_back(std::move(ctr));
		}

		template<CanReflect T, typename... Args>
		static void RegisterDestructor()
		{
			auto typeInfo = GetInfoForAddition<T>();

			DestructorInfo dtr;
			dtr.parent_type = typeInfo;
			dtr.invoker = [](Instance object) {
				if (object.type->name != TypeTraits<T>::LongName)
					return;
				delete static_cast<T*>(object.data);
			};

			typeInfo->destructor.emplace(std::move(dtr));
		}

		template<CanReflect T, typename MemberType>
		static void RegisterMember(std::string_view name, MemberType accessor)
		{
			if constexpr (std::is_member_object_pointer_v<MemberType>)
				RegisterProperty<T, MemberType>(name, accessor);
			else if constexpr (std::is_member_function_pointer_v<MemberType>)
				RegisterMethod<T, MemberType>(name, accessor);
		}

	private:
		template<CanReflect T>
		static TypeInfo* GetInfoForAddition()
		{
			using BaseType = std::remove_cvref_t<std::remove_pointer_t<T>>;
			using Traits = TypeTraits<BaseType>;

			if (not sReflectionState.data.contains(Traits::LongName))
				Register<BaseType>();

			return &sReflectionState.data[Traits::LongName];
		}
		template<CanReflect T>
		static const TypeInfo* GetInfo()
		{
			using BaseType = std::remove_cvref_t<std::remove_pointer_t<T>>;
			using Traits = TypeTraits<BaseType>;

			if (not sReflectionState.data.contains(Traits::LongName))
				Register<BaseType>();

			return &sReflectionState.data[Traits::LongName];
		}

		template<CanReflect T>
		static void Register()
		{
			SCONSTEXPR bool IsBuiltInType = std::is_arithmetic_v<T>;
			if constexpr (not IsBuiltInType)
			{
				using Traits = TypeTraits<T>;

				TypeInfo new_type;
				new_type.name = Traits::LongName;
				RegisterBaseClasses(new_type, BaseClassList<T>{});

				sReflectionState.data.emplace(Traits::LongName, std::move(new_type));

				if constexpr (std::is_default_constructible_v<T>)
					RegisterConstructor<T>();
				if constexpr (std::is_destructible_v<T>)
					RegisterDestructor<T>();
			}
		}

		template<typename... Ts>
		static void RegisterBaseClasses(TypeInfo& type, TypeList<Ts...>)
		{
			([&]()
			{
				if constexpr (CanReflect<Ts>)
				{
					type.base_types.push_back(GetInfo<Ts>());
				}
			}(), ...);
		}

		template<CanReflect T, typename MemberType>
		static void RegisterProperty(std::string_view name, MemberType accessor)
		{
			using PropType = typename PropertyTraits<decltype(accessor)>::PropType;

			auto typeInfo = GetInfoForAddition<T>();

			PropertyInfo prop;
			prop.name = name;
			prop.parent_type = typeInfo;

			prop.prop_type = GetInfo<PropType>();

			prop.accessor = [accessor](Instance ctx) {
				return Instance(
					GetInfo<PropType>(),
					&(static_cast<T*>(ctx.data)->*accessor)
				);
			};
			prop.const_accessor = [accessor](ConstInstance ctx) {
				return ConstInstance(
					GetInfo<PropType>(),
					&(static_cast<const T*>(ctx.data)->*accessor)
				);
			};

			prop.setter = [accessor](Instance ctx, ConstInstance value) {
				static_cast<T*>(ctx.data)->*accessor = *static_cast<const PropType*>(value.data);
			};

			typeInfo->properties.push_back(std::move(prop));
		}

		template<CanReflect T, typename MemberType>
		static void RegisterMethod(std::string_view name, MemberType accessor)
		{
			auto typeInfo = GetInfoForAddition<T>();

			using MemberTraits = FunctionTraits<MemberType>;
			using ArgTypes = MemberTraits::Arguments;
			using ReturnType = typename MemberTraits::ReturnType;

			auto get_arg_types = [] <std::size_t... Is> (std::index_sequence<Is...>) -> std::vector<const TypeInfo*> {
				return { { GetInfo< MemberTraits::template Arg<Is>::Type>()... } };
			};

			auto arg_types = get_arg_types(std::make_index_sequence<MemberTraits::ArgC>());

			MethodInfo method;
			method.name = name;
			method.parent_type = typeInfo;
			method.return_type = GetInfo<ReturnType>();
			method.arguments = std::move(arg_types);

			auto gen_tuple_val = []<std::size_t I>(Instance object) {
				using ArgType = MemberTraits::template Arg<I>::Type;
				return static_cast<ArgType*>(object.data);
			};

			auto gen_tuple = [gen_tuple_val] <std::size_t... Is> (const std::vector<Instance>&args, std::index_sequence<Is...>)-> ArgTypes::TupleType {
				return std::make_tuple(gen_tuple_val.template operator()<Is>(args[Is])...);
			};

			method.invoker = [=](Instance ctx, const std::vector<Instance>& args) -> InvokeResult {
				auto tuple_params = gen_tuple(args, std::make_index_sequence<MemberTraits::ArgC>());

				if constexpr (std::same_as<ReturnType, void>)
				{
					auto func = [&]<typename... Args>(Args&&... argv) {
						(static_cast<T*>(ctx.data)->*accessor)(std::forward<Args>(argv)...);
					};
					std::apply(func, tuple_params);
					return std::nullopt;
				}
				else
				{
					auto func = [&]<typename... Args>(Args&&... argv) {
						return (static_cast<T*>(ctx.data)->*accessor)(std::forward<Args>(argv)...);
					};

					return Instance(
						GetInfo<ReturnType>(),
						new ReturnType(std::apply(func, tuple_params)),
						[](void* data) { delete static_cast<T*>(data); }
					);
				}
			};

			typeInfo->methods.push_back(std::move(method));
		}

	private:
		SCONSTEXPR std::string_view BuiltInType = "__BuiltIn__";
		using ReflectionData = std::unordered_map<std::string_view, TypeInfo>;

		struct Impl
		{
			ReflectionData data = { { BuiltInType, {} } };
			std::vector<std::function<void()>> init_job_queue;

			Impl() : data{ { BuiltInType, {} } }
			{}
		};

		inline static Impl sReflectionState;

		template<typename T>
		friend struct Reflectable;
	};

	template<typename T>
	inline Instance Reflectable<T>::GetInstance()
	{
		return Instance(
			Reflection::GetInfo<T>(),
			this
		);
	}

	template<typename T>
	inline ConstInstance Reflectable<T>::GetConstInstance() const
	{
		return ConstInstance(
			Reflection::GetInfo<T>(),
			this
		);
	}

	template<CanReflect T>
	inline T* Instance::As()
	{
		SCONSTEXPR bool IsBuiltInType = std::is_arithmetic_v<T>;

		if constexpr (not IsBuiltInType)
		{
			using Traits = TypeTraits<T>;
			if (type->name != Traits::LongName)
				return nullptr;
		}

		return static_cast<T*>(data);
	}

	template<CanReflect T>
	inline const T* ConstInstance::As()
	{
		SCONSTEXPR bool IsBuiltInType = std::is_arithmetic_v<T>;

		if constexpr (not IsBuiltInType)
		{
			using Traits = TypeTraits<T>;
			if (type->name != Traits::LongName)
				return nullptr;
		}

		return static_cast<const T*>(data);
	}

#define SLC_REFLECT_MEMBER(CLASS, member)											\
{																					\
	using MemberType = decltype(&CLASS::member);									\
	::slc::Reflection::RegisterMember<CLASS, MemberType>(#member, &CLASS::member);	\
}

#define SLC_DEFER_REFLECT(CLASS, ...)			\
	::slc::Reflection::QueueAddType([]() {		\
		SLC_REFLECT_MEMBER(CLASS, __VA_ARGS__)	\
	})
}
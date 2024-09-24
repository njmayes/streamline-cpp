#pragma once

#include "Core.h"
#include "Exception.h"

namespace slc {

	class Reflection
	{
	public:
		using AddTypeJob = std::function<void()>;
		static void QueueReflection(AddTypeJob&& job)
		{
			sReflectionState.init_jobs.push_back(std::move(job));
		}

		static void ProcessQueue()
		{
			for (const auto& add_type : sReflectionState.init_jobs)
			{
				add_type();
			}
		}

		template<CanReflect T>
		static const TypeInfo* GetInfo()
		{
			using Traits = TypeTraits<T>;

			if (not sReflectionState.data.contains(Traits::Name))
				Register<T>();

			return &sReflectionState.data[Traits::Name];
		}

		static const TypeInfo* GetInfo(std::string_view type_name)
		{
			if (not sReflectionState.data.contains(type_name))
				return nullptr;

			return &sReflectionState.data[type_name];
		}

		template<CanReflect T, typename... Args> requires std::is_constructible_v<T, Args...>
		static void RegisterConstructor(Ctr<T, Args...> = {})
		{
			auto typeInfo = GetInfoForAddition<T>();

			using Params = TypeList<Args...>;

			ConstructorInfo ctr;
			ctr.parent_type = typeInfo;
			ctr.arguments = { GetInfo<Args>()... };

			auto gen_tuple_val = []<std::size_t I>(Instance& object) {
				using ArgType = typename Params::template Type<I>;
				return object.data.Get<ArgType>();
			};

			auto gen_tuple = [gen_tuple_val] <std::size_t... Is> (std::vector<Instance>& args, std::index_sequence<Is...>)-> Params::TupleType {
				return std::make_tuple(gen_tuple_val.template operator()<Is>(args[Is])...);
			};

			ctr.invoker = [gen_tuple](std::vector<Instance> args = {}) {
				auto tuple_params = gen_tuple(args, std::make_index_sequence<Params::Size>());

				auto ctr_func = [](Args&&... args) {
					return T(std::forward<Args>(args)...);
				};

				return Instance(
					GetInfo<T>(),
					std::apply(ctr_func, std::move(tuple_params))
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
				if (object.type->name != TypeTraits<T>::Name)
					return;
				object.data.Get<T&>().~T();
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
			using Traits = TypeTraits<T>;

			if (not sReflectionState.data.contains(Traits::Name))
				Register<T>();

			return &sReflectionState.data[Traits::Name];
		}

		template<CanReflect T>
		static void Register()
		{
			SCONSTEXPR bool IsReflectableType = std::derived_from<T, Reflectable<T>>;
			if constexpr (IsReflectableType)
			{
				using Traits = TypeTraits<T>;

				TypeInfo new_type;
				new_type.name = Traits::Name;
				new_type.base_name = Traits::BaseName;
				new_type.rttt.Init<T>();
				RegisterBaseClasses<T>(new_type, BaseClassList<T>{});

				sReflectionState.data.emplace(Traits::Name, std::move(new_type));

				if constexpr (std::is_default_constructible_v<T>)
					RegisterConstructor<T>();
				if constexpr (std::is_copy_constructible_v<T>)
					RegisterConstructor<T, const T&>();
				if constexpr (std::is_move_constructible_v<T>)
					RegisterConstructor<T, T&&>();

				if constexpr (std::is_destructible_v<T>)
					RegisterDestructor<T>();

				T::BuildReflectionData();
			}
			else
			{
				using Traits = TypeTraits<T>;

				TypeInfo new_type;
				new_type.name = Traits::Name;
				new_type.base_name = Traits::BaseName;
				new_type.rttt.Init<T>();
				sReflectionState.data.emplace(Traits::Name, std::move(new_type));
			}
		}

		template<typename T, typename... Ts>
		static void RegisterBaseClasses(TypeInfo& type, TypeList<Ts...> ts)
		{
			([&]()
			{
				using BaseType = typename Ts::type;
				if constexpr (not std::same_as<T, BaseType> and CanReflect<BaseType>)
				{
					type.base_types.push_back(GetInfo<BaseType>());
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
					ctx.data.Get<const T&>().*accessor
				);
			};

			prop.setter = [accessor](Instance ctx, Instance value) {
				ctx.data.Get<T&>().*accessor = value.data.Get<PropType>();
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

			SCONSTEXPR bool IsReturnVoid = std::same_as<ReturnType, void>;

			auto get_arg_types = [] <std::size_t... Is> (std::index_sequence<Is...>) -> std::vector<const TypeInfo*> {
				return { GetInfo< ArgTypes::template Type<Is>>()... };
			};

			auto arg_types = get_arg_types(std::make_index_sequence<ArgTypes::Size>());

			MethodInfo method;
			method.name = name;
			method.parent_type = typeInfo;
			method.arguments = std::move(arg_types);
			if constexpr (not IsReturnVoid)
				method.return_type = GetInfo<ReturnType>();

			auto gen_tuple_val = []<std::size_t I>(Instance& object) {
				using ArgType = ArgTypes::template Type<I>;
				return object.data.Get<ArgType>();
			};

			auto gen_tuple = [gen_tuple_val] <std::size_t... Is> (std::vector<Instance>& args, std::index_sequence<Is...>) {
				return std::make_tuple(gen_tuple_val.template operator()<Is>(args[Is])...);
			};

			method.invoker = [=](Instance ctx, std::vector<Instance> args = {}) -> Instance {
				auto tuple_params = gen_tuple(args, std::make_index_sequence<ArgTypes::Size>());

				if constexpr (IsReturnVoid)
				{
					auto func = [&]<typename... Args>(Args&&... argv) {
						auto& ctx_ref = ctx.data.Get<T&>();
						(ctx_ref.*accessor)(std::forward<Args>(argv)...);
					};
					std::apply(func, tuple_params);
					return {};
				}
				else
				{
					auto func = [&]<typename... Args>(Args&&... argv) -> ReturnType {
						auto& ctx_ref = ctx.data.Get<T&>();
						return (ctx_ref.*accessor)(std::forward<Args>(argv)...);
					};

					return Instance(
						GetInfo<ReturnType>(),
						std::apply(func, std::move(tuple_params))
					);
				}
				};

			typeInfo->methods.push_back(std::move(method));
		}

	private:
		using ReflectionData = std::unordered_map<std::string_view, TypeInfo>;
		using AddJobQueue = std::vector<AddTypeJob>;

		struct Impl
		{
			ReflectionData data;
			AddJobQueue init_jobs;
		};

		inline static Impl sReflectionState;
	};

	template<CanReflect T>
	inline Instance MakeInstance(T&& value)
	{
		return Instance(
			Reflection::GetInfo<T>(),
			std::forward<T>(value)
		);
	}
}

#define SLC_REFLECT_MEMBER_IMPL(member)															\
    {																							\
		auto invoker = []<typename T>{															\
			if constexpr (std::derived_from<T, ::slc::CtrBase>)						            \
				::slc::Reflection::RegisterConstructor<ClassType>(T{});							\
			else																				\
				::slc::Reflection::RegisterMember<ClassType>(#member, &ClassType::member);		\
		};																						\
        using MemberType = decltype(ClassType::template member);								\
        invoker.template operator()<MemberType>();											    \
    }

#define SLC_REFLECT_CLASS(CLASS, ...)						\
    using Reflectable<CLASS>::Ctr;                          \
    using Reflectable<CLASS>::ArgumentType;                 \
    static void BuildReflectionData()		                \
	{														\
		using ClassType = CLASS;							\
		SLC_FOR_EACH(SLC_REFLECT_MEMBER_IMPL, __VA_ARGS__)  \
	}


#define SLC_REMOVE_PAREN(...) ArgumentType<void(__VA_ARGS__)>::type

#define SLC_CTR(...) SLC_REMOVE_PAREN(Ctr<__VA_ARGS__>){}

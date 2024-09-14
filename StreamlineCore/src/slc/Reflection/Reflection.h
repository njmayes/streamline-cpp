#pragma once

#include <slc/Common/Base.h>

namespace slc {

	namespace detail {

		template <typename T>
		struct tag
		{
			using type = T;
		};

		constexpr void adl_ViewBase() {} // A dummy ADL target.

		template <typename D, std::size_t I>
		struct BaseViewer
		{
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-template-friend"
#endif
			friend constexpr auto adl_ViewBase(BaseViewer);
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
		};

		template <typename D, std::size_t I, typename B>
		struct BaseWriter
		{
			friend constexpr auto adl_ViewBase(BaseViewer<D, I>) { return tag<B>{}; }
		};

		template <typename D, typename Unique, std::size_t I = 0, typename = void>
		struct NumBases : std::integral_constant<std::size_t, I> {};

		template <typename D, typename Unique, std::size_t I>
		struct NumBases < D, Unique, I, decltype(adl_ViewBase(BaseViewer<D, I>{}), void()) > : std::integral_constant<std::size_t, NumBases<D, Unique, I + 1, void>::value> {};

		template <typename D, typename B>
		struct BaseInserter : BaseWriter<D, NumBases<D, B>::value, B> {};

		template <typename T>
		constexpr void adl_RegisterBases(void*) {} // A dummy ADL target.

		template <typename T>
		struct RegisterBases : decltype(adl_RegisterBases<T>((T*)nullptr), tag<void>())
		{};

		template <typename T, typename I>
		struct BaseListLow {};

		template <typename T, std::size_t ...I>
		struct BaseListLow<T, std::index_sequence<I...>>
		{
			static constexpr TypeList<decltype(adl_ViewBase(BaseViewer<T, I>{}))... > helper() {}
			using type = decltype(helper());
		};

		template <typename T>
		struct BaseList : BaseListLow < T, std::make_index_sequence < (RegisterBases<T>{}, NumBases<T, void>::value) >> {};
	}

	template <typename T>
	using BaseClassList = detail::BaseList<T>::type;

	struct TypeInfo;
	struct FunctionInfo;
	struct ConstructorInfo;
	struct DestructorInfo;
	struct PropertyInfo;
	struct MethodInfo;

	struct Instance
	{
		const TypeInfo* type;
		void* data;

		std::function<void(void*)> deleter;

		~Instance()
		{
			if (deleter)
				deleter(data);
		}
	};

	struct ConstInstance
	{
		const TypeInfo* type;
		const void* data;

		std::function<void(const void*)> deleter;

		~ConstInstance()
		{
			if (deleter)
				deleter(data);
		}
	};

	using InvokeResult = std::optional<Instance>;

	using GetFunction = std::function<Instance (Instance)>;
	using ConstGetFunction = std::function<ConstInstance (ConstInstance)>;
	using SetFunction = std::function<void(Instance, ConstInstance)>;

	using Constructor = std::function<Instance(const std::vector<Instance>&)>;
	using Destructor = std::function<void(Instance)>;
	using Invoker = std::function<InvokeResult(Instance, std::vector<Instance> const&)>;

	struct PropertyInfo
	{
		std::string_view name;
		const TypeInfo* parent_type;
		const TypeInfo* prop_type;
		GetFunction accessor;
		ConstGetFunction const_accessor;
		SetFunction setter;
	};

	struct FunctionInfo
	{
		const TypeInfo* return_type;
		std::vector<const TypeInfo*> arguments;
		Invoker invoker;
	};

	struct MethodInfo
	{
		const TypeInfo* parent_type;
		const TypeInfo* return_type;
		std::vector<const TypeInfo*> arguments;
		Invoker invoker;
	};

	struct ConstructorInfo
	{
		const TypeInfo* parent_type;
		Constructor invoker;
	};

	struct DestructorInfo
	{
		const TypeInfo* parent_type;
		Destructor invoker;
	};

	struct TypeInfo
	{
		std::string_view name;
		std::vector<const TypeInfo*> base_types;
		std::vector<ConstructorInfo> constructors;
		std::optional<DestructorInfo> destructor;
		std::vector<MethodInfo> methods;
		std::vector<PropertyInfo> properties;
	};

	struct RuntimeTypeTraits
	{
		bool is_void;
		bool is_null_pointer;
		bool is_integral;
		bool is_floating_point;
		bool is_array;
		bool is_pointer;
		bool is_lvalue_reference;
		bool is_rvalue_reference;
		bool is_member_object_pointer;
		bool is_member_function_pointer;
		bool is_enum;
		bool is_union;
		bool is_class;
		bool is_function;
		bool is_reference;
		bool is_arithmetic;
		bool is_fundamental;
		bool is_object;
		bool is_scalar;
		bool is_compound;
		bool is_member_pointer;
		bool is_const;
		bool is_volatile;
		bool is_trivial;
		bool is_trivially_copyable;
		bool is_standard_layout;
		bool is_empty;
		bool is_polymorphic;
		bool is_abstract;
		bool is_final;
		bool is_aggregate;
		bool is_signed;
		bool is_unsigned;
		bool is_bounded_array;
		bool is_unbounded_array;
		bool is_scoped_enum;

		template<typename T>
		void Init()
		{
			is_void = std::is_void_v<T>;
			is_null_pointer = std::is_null_pointer_v<T>;
			is_integral = std::is_integral_v<T>;
			is_floating_point = std::is_floating_point_v<T>;
			is_array = std::is_array_v<T>;
			is_pointer = std::is_pointer_v<T>;
			is_lvalue_reference = std::is_lvalue_reference_v<T>;
			is_rvalue_reference = std::is_rvalue_reference_v<T>;
			is_member_object_pointer = std::is_member_object_pointer_v<T>;
			is_member_function_pointer = std::is_member_function_pointer_v<T>;
			is_enum = std::is_enum_v<T>;
			is_union = std::is_union_v<T>;
			is_class = std::is_class_v<T>;
			is_function = std::is_function_v<T>;
			is_reference = std::is_reference_v<T>;
			is_arithmetic = std::is_arithmetic_v<T>;
			is_fundamental = std::is_fundamental_v<T>;
			is_object = std::is_object_v<T>;
			is_scalar = std::is_scalar_v<T>;
			is_compound = std::is_compound_v<T>;
			is_member_pointer = std::is_member_pointer_v<T>;
			is_const = std::is_const_v<T>;
			is_volatile = std::is_volatile_v<T>;
			is_trivial = std::is_trivial_v<T>;
			is_trivially_copyable = std::is_trivially_copyable_v<T>;
			is_standard_layout = std::is_standard_layout_v<T>;
			is_empty = std::is_empty_v<T>;
			is_polymorphic = std::is_polymorphic_v<T>;
			is_abstract = std::is_abstract_v<T>;
			is_final = std::is_final_v<T>;
			is_aggregate = std::is_aggregate_v<T>;
			is_signed = std::is_signed_v<T>;
			is_unsigned = std::is_unsigned_v<T>;
			is_bounded_array = std::is_bounded_array_v<T>;
			is_unbounded_array = std::is_unbounded_array_v<T>;
			is_scoped_enum = std::is_scoped_enum_v<T>;
		}
	};

	template <typename T>
	struct ReflectBase
	{
		template <
			typename D,
			std::enable_if_t<std::is_base_of_v<T, D>, std::nullptr_t> = nullptr,
			typename detail::BaseInserter<D, T>::nonExistent = nullptr
		>
		friend constexpr void adl_RegisterBases(void*) {}

		Instance GetInstance();
		ConstInstance GetConstInstance() const;
	};

	template<typename T>
	concept Reflectable = std::derived_from<T, ReflectBase<T>>
		or std::is_arithmetic_v<T>
		or std::is_pointer_v<T>
		or std::is_reference_v<T>;

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

		template<Reflectable T>
		static const TypeInfo& Get()
		{
			using BaseType = std::remove_cvref_t<std::remove_pointer_t<T>>;
			using Traits = TypeTraits<BaseType>;

			SCONSTEXPR bool IsBuiltInType = std::is_arithmetic_v<BaseType>;
			SCONSTEXPR std::string_view Key = IsBuiltInType ? BuiltInType : Traits::LongName;

			using Traits = TypeTraits<BaseType>;
			if (not sReflectionState.data.contains(Traits::LongName))
				Register<BaseType>();

			return sReflectionState.data[Key];
		}

		template<Reflectable T, typename... Args>
		static void RegisterConstructor()
		{
			auto typeInfo = ::slc::Reflection::GetForAddition<T>();

			using Params = TypeList<Args...>;

			ConstructorInfo ctr;
			ctr.parent_type = typeInfo;

			auto gen_tuple_val = []<std::size_t I>(Instance object) {
				using ArgType = typename Params::template Type<I>;
				return static_cast<ArgType*>(object.data);
			};

			auto gen_tuple = [gen_tuple_val] <std::size_t... Is> (const std::vector<Instance>&args, std::index_sequence<Is...>)-> Params::TupleType {
				return std::make_tuple(gen_tuple_val.operator()<Is>(args[Is])...);
			};

			ctr.invoker = [gen_tuple](const std::vector<Instance>& args) {
				auto tuple_params = gen_tuple(args, std::make_index_sequence<Params::Size>());

				auto ctr = []<typename... Ts>(Ts&&... args) {
					return new T(std::forward<Ts>(args)...);
				};

				Instance object;
				object.type = &Get<T>();
				object.data = std::apply(ctr, tuple_params);
				object.deleter = [](void* data) { delete static_cast<T*>(data); };
				return object;
			};

			typeInfo->constructors.push_back(std::move(ctr));
		}

		template<Reflectable T, typename... Args>
		static void RegisterDestructor()
		{
			auto typeInfo = ::slc::Reflection::GetForAddition<T>();

			DestructorInfo dtr;
			dtr.parent_type = typeInfo;
			dtr.invoker = [](Instance object) {
				if (object.type->name != TypeTraits<T>::LongName)
					return;
				delete static_cast<T*>(object.data);
			};

			typeInfo->destructor.emplace(std::move(dtr));
		}

		template<Reflectable T, typename MemberType>
		static void RegisterMember(std::string_view name, MemberType accessor)
		{
			if constexpr (std::is_member_object_pointer_v<MemberType>)
				RegisterProperty<T, MemberType>(name, accessor);
			else if constexpr (std::is_member_function_pointer_v<MemberType>)
				RegisterMethod<T, MemberType>(name, accessor);
		}

	private:
		template<Reflectable T>
		static TypeInfo* GetForAddition()
		{
			using BaseType = std::remove_cvref_t<std::remove_pointer_t<T>>;
			using Traits = TypeTraits<BaseType>;

			return &sReflectionState.data[Traits::LongName];
		}

		template<Reflectable T>
		static void Register()
		{
			SCONSTEXPR bool IsBuiltInType = std::is_arithmetic_v<T>;
			if constexpr (not IsBuiltInType)
			{
				using Traits = TypeTraits<T>;

				auto& type = sReflectionState.data[Traits::LongName];

				type.name = Traits::LongName;
				RegisterBaseClasses(type, BaseClassList<T>{});
			}
		}

		template<typename... Ts>
		static void RegisterBaseClasses(TypeInfo& type, TypeList<Ts...>)
		{
			([&]()
			{
				if constexpr (Reflectable<Ts>)
				{
					type.base_types.push_back(&Get<Ts>());
				}
			}(), ...);
		}

		static void RegisterBuiltInTypes()
		{
			sReflectionState.data[BuiltInType];
		}

		template<Reflectable T, typename MemberType>
		static void RegisterProperty(std::string_view name, MemberType accessor)
		{
			using PropType = typename PropertyTraits<decltype(accessor)>::PropType;

			auto typeInfo = GetForAddition<T>();

			PropertyInfo prop;
			prop.name = name;
			prop.parent_type = typeInfo;

			prop.prop_type = &Get<PropType>();

			prop.accessor = [accessor](Instance ctx) {
				return Instance{
					.type = &Get<PropType>(),
					.data = &(static_cast<T*>(ctx.data)->*accessor),
				};
			};
			prop.const_accessor = [accessor](ConstInstance ctx) {
				return ConstInstance{
					.type = &Get<PropType>(),
					.data = &(static_cast<const T*>(ctx.data)->*accessor),
				};
			};

			prop.setter = [accessor](Instance ctx, ConstInstance value) {
				static_cast<T*>(ctx.data)->*accessor = *static_cast<const PropType*>(value.data);
			};

			typeInfo->properties.push_back(std::move(prop));
		}

		template<Reflectable T, typename MemberType>
		static void RegisterMethod(std::string_view name, MemberType accessor)
		{
			auto typeInfo = GetForAddition<T>();

			using MemberTraits = FunctionTraits<MemberType>;
			using ArgTypes = MemberTraits::Arguments;
			using ReturnType = typename MemberTraits::ReturnType;

			auto get_arg_type = []<std::size_t I>() {
				using ArgType = MemberTraits::template Arg<I>::Type;
				return Get<ArgType>();
			};

			auto get_arg_types = [&] <std::size_t... Is> (std::index_sequence<Is...>) -> std::vector<const TypeInfo*> {
				return { { Get< MemberTraits::template Arg<Is>::Type>()... } };
			};

			auto arg_types = get_arg_types(std::make_index_sequence<MemberTraits::ArgC>());

			MethodInfo method;
			method.parent_type = typeInfo;
			method.return_type = &Get<ReturnType>();
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
					Instance result;
					result.type = &Get<ReturnType>();
					auto func = [&]<typename... Args>(Args&&... argv) {
						return (static_cast<T*>(ctx.data)->*accessor)(std::forward<Args>(argv)...);
					};
					result.data = new ReturnType(std::apply(func, tuple_params));
					result.deleter = [](void* data) { delete static_cast<T*>(data); };

					return result;
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
	};

	template<typename T>
	inline Instance ReflectBase<T>::GetInstance()
	{
		return Instance{
			.type = &Reflection::Get<T>(),
			.data = this,
		};
	}

	template<typename T>
	inline ConstInstance ReflectBase<T>::GetConstInstance() const
	{
		return ConstInstance{
			.type = &Reflection::Get<T>(),
			.data = this,
		};
	}

	template<typename T>
	T* CastInstance(Instance object)
	{
		if (object.type->name != TypeTraits<T>::LongName)
			return nullptr;

		return static_cast<T*>(object.data);
	}

#define SLC_REFLECT_TYPE(CLASS)							\
{														\
	::slc::Reflection::RegisterConstructor<CLASS>();	\
	::slc::Reflection::RegisterDestructor<CLASS>();		\
}

#define SLC_REFLECT_MEMBER(CLASS, member)											\
{																					\
	using MemberType = decltype(&CLASS::member);									\
	::slc::Reflection::RegisterMember<CLASS, MemberType>(#member, &CLASS::member);	\
}

#define SLC_DEFER_REFLECT(CLASS, ...)			\
	::slc::Reflection::QueueAddType([]() {		\
		SLC_REFLECT_TYPE(CLASS)					\
		SLC_REFLECT_MEMBER(CLASS, __VA_ARGS__)	\
	})
}
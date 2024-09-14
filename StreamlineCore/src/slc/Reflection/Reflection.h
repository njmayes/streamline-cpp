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
	using BaseClassList = typename detail::BaseList<T>::type;

	template <typename T>
	struct ReflectBase
	{
		template <
			typename D,
			std::enable_if_t<std::is_base_of_v<T, D>, std::nullptr_t> = nullptr,
			typename detail::BaseInserter<D, T>::nonExistent = nullptr
		>
		friend constexpr void adl_RegisterBases(void*) {}
	};

	template<typename T>
	concept Reflectable = std::derived_from<T, ReflectBase<T>>
		or std::is_arithmetic_v<T>
		or std::is_pointer_v<T>
		or std::is_reference_v<T>;

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
	};

	using GetFunction = std::function<const void* (const void*)>;
	using SetFunction = std::function<void(void*, const void*)>;

	template<typename... Args>
	using Constructor = std::function<void*(Args&&...)>;
	using Destructor = std::function<void(void*)>;
	using Invoker = std::function<void*(void*, std::vector<Instance> const&)>;

	struct PropertyInfo
	{
		std::string_view name;
		const TypeInfo* parent_type;
		const TypeInfo* prop_type;
		GetFunction accessor;
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
		Constructor<> invoker;
	};

	struct DestructorInfo
	{
		const TypeInfo* parent_type;
		Destructor invoker;
	};

	struct TypeInfo : public RefCounted
	{
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

	using AddTypeJob = std::function<void()>;

	class Reflection
	{
	public:
		static void QueueAddType(AddTypeJob&& job)
		{
			if (sReflectionState.initialised)
				return;

			sReflectionState.init_job_queue.push_back(std::move(job));
		}

		static void Init() 
		{
			RegisterBuiltInTypes();

			for (const auto& add_type : sReflectionState.init_job_queue)
			{
				add_type();
			}

			sReflectionState.initialised = true;
		}
		static void Shutdown()
		{
			sReflectionState.data.clear();
			sReflectionState.initialised = false;
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

		template<Reflectable T>
		static TypeInfo* GetForAddition()
		{
			if (sReflectionState.initialised)
				return nullptr;

			using BaseType = std::remove_cvref_t<std::remove_pointer_t<T>>;
			using Traits = TypeTraits<BaseType>;

			return &sReflectionState.data[Traits::LongName];
		}

		template<Reflectable T, typename MemberType, typename PropType>
		static void RegisterMember(std::string_view name, MemberType accessor)
		{
			auto typeInfo = GetForAddition<T>();
			if constexpr (std::is_member_object_pointer_v<MemberType>)
			{
				PropertyInfo prop;
				prop.name = name;
				prop.parent_type = typeInfo;

				prop.prop_type = &Get<PropType>();

				prop.accessor = [accessor](const void* ctx) {
					return &(static_cast<const T*>(ctx)->*accessor);
					};

				prop.setter = [accessor](void* ctx, const void* value) {
					static_cast<T*>(ctx)->*accessor = *static_cast<const PropType*>(value);
				};

				typeInfo->properties.push_back(std::move(prop));
			}
			else if constexpr (std::is_member_function_pointer_v<MemberType>)
			{
				using MemberTraits = FunctionTraits<MemberType>;
				using ArgTypes = MemberTraits::Arguments;

				auto get_arg_type = []<std::size_t I>() {
					using ArgType = MemberTraits::template Arg<I>::Type;
					return Get<ArgType>();
				};

				auto get_arg_types = [&] <std::size_t... Is> (std::index_sequence<Is...>) -> std::vector<const TypeInfo*> {
					return { { Get< MemberTraits::template Arg<Is>::Type>()... } };
				};

				auto arg_types = std::vector<const TypeInfo*>{};
				arg_types.push_back(typeInfo);
				arg_types.append_range(get_arg_types(std::make_index_sequence<MemberTraits::ArgC>()));

				MethodInfo method;
				method.parent_type = typeInfo;
				method.return_type = &Get<MemberTraits::template ReturnType>();
				method.arguments = std::move(arg_types);

				auto get_arg_val = []<std::size_t I>(void* val) {
					using ArgType = MemberTraits::template Arg<I>::Type;
					return static_cast<ArgType*>(val);
				};

				auto gen_tuple_val = []<std::size_t I>(Instance object) {
					using ArgType = MemberTraits::template Arg<I>::Type;

				};

				auto gen_tuple = [&]<std::size_t... Is> (const std::vector<Instance>&args, std::index_sequence<Is...>)-> ArgTypes::TupleType {
					return std::make_tuple(gen_tuple_val.operator()<Is>(args[Is])...);
				};

				method.invoker = [&](void* ctx, const std::vector<Instance>& args) -> void* {
					auto tuple_params = gen_tuple(args, std::make_index_sequence<MemberTraits::ArgC>());

					if constexpr (std::same_as<MemberTraits::template ReturnType, void>)
					{
						auto func = [&]<typename... Args>(Args&&... argv) {
							return (static_cast<T*>(ctx)->*accessor)(std::forward<Args>(argv)...);
						};
						std::apply(func, tuple_params);
						return nullptr;
					}
					else
					{
						auto func = [&]<typename... Args>(Args&&... argv) {
							return (static_cast<T*>(ctx)->*accessor)(std::forward<Args>(argv)...);
						};
						std::apply(func, tuple_params);
						return nullptr;
					}
				};

				typeInfo->methods.push_back(std::move(method));
			}
		}

	private:
		template<Reflectable T>
		static void Register()
		{
			SCONSTEXPR bool IsBuiltInType = std::is_arithmetic_v<T>;
			if constexpr (not IsBuiltInType)
			{
				using Traits = TypeTraits<T>;

				auto& type = sReflectionState.data[Key];

				type->name = Key;
				RegisterBaseClasses(type, BaseClassList<T>{});
			}
		}

		template<Reflectable... Ts>
		static void RegisterBaseClasses(TypeInfo& type, TypeList<Ts...>)
		{
			([&]()
			{
				type.base_types.emplace_back(&Get<Ts>());
			}(), ...);
		}

		static void RegisterBuiltInTypes()
		{
			sReflectionState.data[BuiltInType];
		}

	private:
		SCONSTEXPR std::string_view BuiltInType = "__BuiltIn__";
		using ReflectionData = std::unordered_map<std::string_view, TypeInfo>;

		struct Impl
		{
			std::atomic_bool initialised = false;
			ReflectionData data;
			std::vector<std::function<void()>> init_job_queue;
		};

		inline static Impl sReflectionState;

		template<typename T>
		friend struct ReflectBase;
	};

#define REFLECT_TYPE(CLASS)														\
{																				\
	auto typeInfo = ::slc::Reflection::GetForAddition<CLASS>();			\
	ConstructorInfo ctr;														\
	ctr.parent_type = typeInfo;													\
	ctr.invoker = [] { return new CLASS(); };									\
	typeInfo->constructors.push_back(std::move(ctr));							\
																				\
	DestructorInfo dtr;															\
	dtr.parent_type = typeInfo;													\
	dtr.invoker = [] (void* ptr) { return delete static_cast<CLASS*>(ptr); };	\
	typeInfo->destructor.emplace(std::move(dtr));								\
}

#define REFLECT_MEMBER(CLASS, member)																	\
{																										\
	using MemberType = decltype(&CLASS::member);														\
	using PropType = std::conditional_t<std::is_member_function_pointer_v<MemberType>,					\
		decltype(CLASS::member), std::monostate>;														\
	::slc::Reflection::RegisterMember<CLASS, MemberType, PropType>(#member, &CLASS::member);	\
}

#define REFLECT(CLASS, ...)							\
	::slc::Reflection::QueueAddType([]() {	\
		REFLECT_TYPE(CLASS)							\
		REFLECT_MEMBER(CLASS, __VA_ARGS__)			\
	})

	struct Test : ReflectBase<Test>
	{
		int a, b;

		Test()
		{

		}

		int f() { return a; }
	};

	void func()
	{
		REFLECT(Test, f);
	}
}
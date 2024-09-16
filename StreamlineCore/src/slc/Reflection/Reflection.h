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

	struct Instance;
	struct ConstInstance;

	struct ReflectableObject
	{
		virtual ~ReflectableObject() {}
	};

	template <typename T>
	struct Reflectable : ReflectableObject
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
	concept CanReflect = std::derived_from<T, Reflectable<T>>
		or std::is_arithmetic_v<T>
		or std::is_pointer_v<T>
		or std::is_reference_v<T>;

	struct Instance
	{
		const TypeInfo* type;
		void* data;

		using Deleter = std::function<void(void*)>;
		Deleter deleter;

		Instance(const TypeInfo* t, void* d, Deleter del = {})
			: type(t), data(d), deleter(del) {}

		~Instance()
		{
			if (deleter)
				deleter(data);
		}

		Instance(const Instance&) = delete;
		Instance& operator=(const Instance&) = delete;

		Instance(Instance&& other) 
		{
			type = other.type;
			data = std::exchange(other.data, nullptr);
			deleter = std::move(other.deleter);
		}
		Instance& operator=(Instance&& other)
		{
			type = other.type;
			data = std::exchange(other.data, nullptr);
			deleter = std::move(other.deleter);

			return *this;
		}

		template<CanReflect T>
		T* As();
	};

	struct ConstInstance
	{
		const TypeInfo* type;
		const void* data;

		using Deleter = std::function<void(const void*)>;
		Deleter deleter;

		ConstInstance(const TypeInfo* t, const void* d, Deleter del = {})
			: type(t), data(d), deleter(del) {}

		~ConstInstance()
		{
			if (deleter)
				deleter(data);
		}

		ConstInstance(const ConstInstance&) = delete;
		ConstInstance& operator=(const ConstInstance&) = delete;

		ConstInstance(ConstInstance&& other)
		{
			type = other.type;
			data = std::exchange(other.data, nullptr);
			deleter = std::move(other.deleter);
		}
		ConstInstance& operator=(ConstInstance&& other)
		{
			type = other.type;
			data = std::exchange(other.data, nullptr);
			deleter = std::move(other.deleter);

			return *this;
		}

		template<CanReflect T>
		const T* As();
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
		std::string_view name;
		const TypeInfo* return_type;
		std::vector<const TypeInfo*> arguments;
		Invoker invoker;
	};

	struct MethodInfo
	{
		std::string_view name;
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

	class Property
	{
	public:
		Property() = default;
		Property(const PropertyInfo& info)
			: mProperty(&info)
		{}

		std::string_view GetName() const { return mProperty->name; }

		template<CanReflect T, CanReflect Obj>
		const T& GetValue(const Obj& obj) const
		{
			using Traits = TypeTraits<T>;
			using ObjTraits = TypeTraits<Obj>;
			ASSERT(Traits::LongName == mProperty->prop_type->name);
			ASSERT(ObjTraits::LongName == mProperty->parent_type->name);

			auto instance = GetValue(obj.GetConstInstance());
			ASSERT(instance.type == mProperty->prop_type);

			auto ptr = static_cast<const T*>(instance.data);
			return static_cast<const T&>(*ptr);
		}

		ConstInstance GetValue(ConstInstance obj) const
		{
			return mProperty->const_accessor(std::move(obj));
		}

		template<CanReflect T, CanReflect Obj>
		void SetValue(const Obj& obj, const T& value)
		{
			using Traits = TypeTraits<T>;
			ASSERT(Traits::LongName == mProperty->prop_type->name);

			SetValue(obj.GetInstance(), value.GetConstInstance());
		}

		void SetValue(Instance obj, ConstInstance value)
		{
			return mProperty->setter(std::move(obj), std::move(value));
		}

	private:
		const PropertyInfo* mProperty;
	};

	class Type
	{
	public:
		Type(const TypeInfo& type)
			: mInfo(&type) {}

		Property GetProperty(std::string_view name) const
		{
			auto it = std::ranges::find_if(mInfo->properties, [name](const auto& prop) { return prop.name == name; });
			if (it == mInfo->properties.end())
				return {};

			return Property(*it);
		}

		auto GetProperties() const
		{
			return mInfo->properties | std::views::transform([](const auto& property) { return Property(property); });
		}

	private:
		const TypeInfo* mInfo;

		friend class Reflection;
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

			return sReflectionState.data[Key];
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

			return &sReflectionState.data[Traits::LongName];
		}
		template<CanReflect T>
		static const TypeInfo* GetInfo()
		{
			using BaseType = std::remove_cvref_t<std::remove_pointer_t<T>>;
			using Traits = TypeTraits<BaseType>;

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
		using Traits = TypeTraits<T>;
		if (type->name != Traits::LongName)
			return nullptr;

		return static_cast<T*>(data);
	}

	template<CanReflect T>
	inline const T* ConstInstance::As()
	{
		using Traits = TypeTraits<T>;
		if (type->name != Traits::LongName)
			return nullptr;

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

	struct Test : Reflectable<Test>
	{
		int a, b;

		Test(int c, int d)
			: a(c), b(d) {}

		int f() { return a; }
	};

	void func()
	{
		SLC_REFLECT_MEMBER(Test, a);
		SLC_REFLECT_MEMBER(Test, b);
		SLC_REFLECT_MEMBER(Test, f);

		Test t(2, 6);

		auto const& refl = Reflection::Get<Test>();
		for (auto const& prop : refl.GetProperties())
		{
			std::cout << prop.GetName() << ": " << *prop.GetValue(t.GetConstInstance()).As<int>() << std::endl;
		}

	}
}
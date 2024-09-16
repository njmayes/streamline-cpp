#pragma once

#include "slc/Common/Base.h"

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

		Instance(Instance&& other) noexcept
		{
			type = other.type;
			data = std::exchange(other.data, nullptr);
			deleter = std::move(other.deleter);
		}
		Instance& operator=(Instance&& other) noexcept
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

		ConstInstance(ConstInstance&& other) noexcept
		{
			type = other.type;
			data = std::exchange(other.data, nullptr);
			deleter = std::move(other.deleter);
		}
		ConstInstance& operator=(ConstInstance&& other) noexcept
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

	using GetFunction = std::function<Instance(Instance)>;
	using ConstGetFunction = std::function<ConstInstance(ConstInstance)>;
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
}
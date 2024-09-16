#pragma once

#include "slc/Common/Base.h"

#include <random>

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

	namespace detail {

		class InstanceSeeder
		{
			static std::uint64_t Generate() { return sUniformDistribution(sEngine); }

			inline static std::random_device sRandomDevice;
			inline static std::mt19937_64 sEngine{ sRandomDevice() };
			inline static std::uniform_int_distribution<std::uint64_t>(sUniformDistribution);

			friend struct Instance;
			friend struct ConstInstance;
		};
	}

	struct InstanceBase
	{
		std::uint64_t id = 0;
		const TypeInfo* type = nullptr;

		bool IsVoid() const { return not id ; }
	};

	struct Instance : InstanceBase
	{
		void* data = nullptr;

		using Deleter = std::function<void(void*)>;
		Deleter deleter = {};

		Instance() = default;
		Instance(const TypeInfo* t, void* d, Deleter del = {})
			: InstanceBase{ detail::InstanceSeeder::Generate(), t }, data(d), deleter(del) {}

		~Instance()
		{
			if (deleter)
				deleter(data);
		}

		Instance(const Instance&) = delete;
		Instance& operator=(const Instance&) = delete;

		Instance(Instance&& other) noexcept
		{
			id = std::exchange(other.id, 0);
			type = std::exchange(other.type, nullptr);
			data = std::exchange(other.data, nullptr);
			deleter = std::exchange(other.deleter, {});
		}
		Instance& operator=(Instance&& other) noexcept
		{
			id = std::exchange(other.id, 0);
			type = std::exchange(other.type, nullptr);
			data = std::exchange(other.data, nullptr);
			deleter = std::exchange(other.deleter, {});

			return *this;
		}

		template<CanReflect T>
		T* As() const;
	};

	struct ConstInstance : InstanceBase
	{
		const void* data;

		using Deleter = std::function<void(const void*)>;
		Deleter deleter;

		ConstInstance() = default;
		ConstInstance(const TypeInfo* t, const void* d, Deleter del = {})
			: InstanceBase{ detail::InstanceSeeder::Generate(), t }, data(d), deleter(del) {}

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
		const T* As() const;
	};

	template<typename T> requires (CanReflect<T> and not std::derived_from<T, Reflectable<T>>)
	Instance MakeBuiltInInstance(T* value);

	template<typename T> requires (CanReflect<T> and not std::derived_from<T, Reflectable<T>>)
	ConstInstance MakeBuiltInInstance(const T* value);

	template<CanReflect T>
	Instance MakeInstance(T& value);

	template<CanReflect T>
	ConstInstance MakeInstance(const T& value);

	using InvokeResult = std::optional<Instance>;

	using GetFunction = std::function<ConstInstance(ConstInstance)>;
	using SetFunction = std::function<void(Instance, ConstInstance)>;

	using Constructor = std::function<Instance(const std::vector<Instance>&)>;
	using Destructor = std::function<void(Instance)>;
	using Invoker = std::function<Instance(Instance, std::vector<Instance> const&)>;

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
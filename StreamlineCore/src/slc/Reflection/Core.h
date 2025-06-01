#pragma once

#include "slc/Common/Base.h"
#include "slc/Types/Any.h"

namespace slc {

	namespace detail {

		template < typename T >
		struct tag
		{
			using type = T;
		};

		constexpr void adl_ViewBase()
		{} // A dummy ADL target.

		template < typename D, std::size_t I >
		struct BaseViewer
		{
#if defined( __GNUC__ ) && !defined( __clang__ )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-template-friend"
#endif
			friend constexpr auto adl_ViewBase( BaseViewer );
#if defined( __GNUC__ ) && !defined( __clang__ )
#pragma GCC diagnostic pop
#endif
		};

		template < typename D, std::size_t I, typename B >
		struct BaseWriter
		{
			friend constexpr auto adl_ViewBase( BaseViewer< D, I > )
			{
				return tag< B >{};
			}
		};

		template < typename D, typename Unique, std::size_t I = 0, typename = void >
		struct NumBases : std::integral_constant< std::size_t, I >
		{};

		template < typename D, typename Unique, std::size_t I >
		struct NumBases< D, Unique, I, decltype( adl_ViewBase( BaseViewer< D, I >{} ), void() ) > : std::integral_constant< std::size_t, NumBases< D, Unique, I + 1, void >::value >
		{};

		template < typename D, typename B >
		struct BaseInserter : BaseWriter< D, NumBases< D, B >::value, B >
		{};

		template < typename T >
		constexpr void adl_RegisterBases( void* )
		{} // A dummy ADL target.

		template < typename T >
		struct RegisterBases : decltype( adl_RegisterBases< T >( ( T* )nullptr ), tag< void >() )
		{};

		template < typename T, typename I >
		struct BaseListLow
		{};

		template < typename T, std::size_t... I >
		struct BaseListLow< T, std::index_sequence< I... > >
		{
			static constexpr TypeList< decltype( adl_ViewBase( BaseViewer< T, I >{} ) )... > helper()
			{}
			using type = decltype( helper() );
		};

#if defined( __GNUC__ ) && defined( __clang__ )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
#endif
		template < typename T >
		struct BaseList : BaseListLow< T, std::make_index_sequence< ( RegisterBases< T >{}, NumBases< T, void >::value ) > >
		{};
#if defined( __GNUC__ ) && defined( __clang__ )
#pragma GCC diagnostic pop
#endif
	} // namespace detail

	template < typename T >
	using BaseClassList = detail::BaseList< T >::type;

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

		bool is_default_constructible;
		bool is_copy_constructible;
		bool is_move_constructible;

		template < typename T >
		void Init()
		{
			is_void = std::is_void_v< T >;
			is_null_pointer = std::is_null_pointer_v< T >;
			is_integral = std::is_integral_v< T >;
			is_floating_point = std::is_floating_point_v< T >;
			is_array = std::is_array_v< T >;
			is_pointer = std::is_pointer_v< T >;
			is_lvalue_reference = std::is_lvalue_reference_v< T >;
			is_rvalue_reference = std::is_rvalue_reference_v< T >;
			is_member_object_pointer = std::is_member_object_pointer_v< T >;
			is_member_function_pointer = std::is_member_function_pointer_v< T >;
			is_enum = std::is_enum_v< T >;
			is_union = std::is_union_v< T >;
			is_class = std::is_class_v< T >;
			is_function = std::is_function_v< T >;
			is_reference = std::is_reference_v< T >;
			is_arithmetic = std::is_arithmetic_v< T >;
			is_fundamental = std::is_fundamental_v< T >;
			is_object = std::is_object_v< T >;
			is_scalar = std::is_scalar_v< T >;
			is_compound = std::is_compound_v< T >;
			is_member_pointer = std::is_member_pointer_v< T >;
			is_const = std::is_const_v< std::remove_reference_t< T > >;
			is_volatile = std::is_volatile_v< T >;
			is_trivial = std::is_trivial_v< T >;
			is_trivially_copyable = std::is_trivially_copyable_v< T >;
			is_standard_layout = std::is_standard_layout_v< T >;
			is_empty = std::is_empty_v< T >;
			is_polymorphic = std::is_polymorphic_v< T >;
			is_abstract = std::is_abstract_v< T >;
			is_final = std::is_final_v< T >;
			is_aggregate = std::is_aggregate_v< T >;
			is_signed = std::is_signed_v< T >;
			is_unsigned = std::is_unsigned_v< T >;
			is_bounded_array = std::is_bounded_array_v< T >;
			is_unbounded_array = std::is_unbounded_array_v< T >;
			is_scoped_enum = std::is_scoped_enum_v< T >;

			is_default_constructible = std::is_default_constructible_v< T >;
			is_copy_constructible = std::is_copy_constructible_v< T >;
			is_move_constructible = std::is_move_constructible_v< T >;
		}
	};

	struct TypeInfo;
	struct FunctionInfo;
	struct ConstructorInfo;
	struct DestructorInfo;
	struct PropertyInfo;
	struct MethodInfo;

	struct Instance;

	namespace detail {

		struct CtrBase
		{};

		template < typename T, typename... Args >
		struct Ctr : CtrBase
		{
			// This is icky, but needed to allow macro to do &Class::member and produce the correct result and not an error.
			// This type should only be used to deduce Constructor type parameters anyway
			Ctr< T, Args... > operator&()
			{
				return Ctr< T, Args... >{};
			}
		};
	} // namespace detail

	template < typename T >
	struct Reflectable
	{
		template <
			typename D,
			std::enable_if_t< std::is_base_of_v< T, D >, std::nullptr_t > = nullptr,
			typename detail::BaseInserter< D, T >::nonExistent = nullptr >
		friend constexpr void adl_RegisterBases( void* )
		{}

	protected:
		template < typename... Args >
		using Ctr = detail::Ctr< T, Args... >;

		template < typename R >
		struct ArgumentType;
		template < typename R, typename U >
		struct ArgumentType< R( U ) >
		{
			using type = U;
		};
	};

	namespace detail {

		template < typename T >
		concept IsReflectableType = std::derived_from< T, Reflectable< T > > and requires {
			{ T::slc_refl_data::Build() } -> std::same_as< void >;
			{ T::slc_refl_data::Info } -> std::same_as< const TypeInfo*& >;
		};

		template < typename T >
		concept IsBuiltInReflectable = std::is_arithmetic_v< T > or std::is_enum_v< T > or std::is_pointer_v< T > or std::is_reference_v< T >;
	} // namespace detail

	template < typename T >
	concept CanReflect = detail::IsReflectableType< T > or detail::IsBuiltInReflectable< T >;

	struct Instance
	{
		const TypeInfo* type = nullptr;
		Any data;

		Instance() = default;

		template < typename T >
		Instance( const TypeInfo* t, T&& d )
			: type( t )
			, data( std::forward< T >( d ) )
		{}

		bool Valid() const
		{
			return type and data.HasValue();
		}
		bool IsVoid() const
		{
			return not Valid();
		}
	};

	template < CanReflect T >
	Instance MakeInstance( T&& value )
	{
		return Instance(
			T::ReflectionData::Info,
			std::forward< T >( value )
		);
	}

	using GetFunction = std::function< Instance( Instance ) >;
	using SetFunction = std::function< void( Instance, Instance ) >;

	using ConstructorInvoker = std::function< Instance( std::vector< Instance > ) >;
	using DestructorInvoker = std::function< void( Instance ) >;
	using FunctionInvoker = std::function< Instance( std::vector< Instance > ) >;
	using MethodInvoker = std::function< Instance( Instance, std::vector< Instance > ) >;

	struct PropertyInfo
	{
		std::string_view name;
		const TypeInfo* parent_type;
		const TypeInfo* prop_type;
		GetFunction accessor;
		SetFunction setter;
	};

	struct MethodInfo
	{
		std::string_view name;
		const TypeInfo* parent_type;
		const TypeInfo* return_type;
		std::vector< const TypeInfo* > arguments;
		MethodInvoker invoker;
	};

	struct ConstructorInfo
	{
		const TypeInfo* parent_type;
		std::vector< const TypeInfo* > arguments;
		ConstructorInvoker invoker;
	};

	struct DestructorInfo
	{
		const TypeInfo* parent_type;
		DestructorInvoker invoker;
	};

	struct TypeInfo
	{
		std::string_view name;
		std::string_view base_name;

		RuntimeTypeTraits rttt;

		std::vector< const TypeInfo* > base_types;
		std::vector< ConstructorInfo > constructors;
		std::optional< DestructorInfo > destructor;
		std::vector< MethodInfo > methods;
		std::vector< PropertyInfo > properties;
	};

	struct FunctionInfo
	{
		std::string_view name;
		const TypeInfo* return_type;
		std::vector< const TypeInfo* > arguments;
		FunctionInvoker invoker;
	};
} // namespace slc
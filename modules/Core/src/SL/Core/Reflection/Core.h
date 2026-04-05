#pragma once

#include "SL/Core/Common/Base.h"
#include "SL/Core/Types/Any.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>
#include <format>

namespace sl {

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
			// Gross, but needed to be able to do `using Foo = declspec( &Bar::baz )` during reflection registration 
			// where baz will either be a class member or this type.
			// Marked consteval to prevent any other usage of this operator, which would be meaningless.
			consteval Ctr< T, Args... > operator&()
			{
				return Ctr< T, Args... >{};
			}
		};
	} // namespace detail

	namespace detail {

		template < typename T >
		concept IsReflectableType = requires {
			{ T::_reflection_data::Build() } -> std::same_as< void >;
			{ T::_reflection_data::Info } -> std::same_as< const TypeInfo*& >;
		};

		template < typename T >
		concept IsBuiltInReflectable = std::is_arithmetic_v< T > or std::is_enum_v< T > or std::is_pointer_v< T > or std::is_reference_v< T >;
	} // namespace detail

	template < typename T >
	concept CanReflect = detail::IsReflectableType< T > or detail::IsBuiltInReflectable< T >;

	// -----------------------------
	// TypeRef: "usage" of a type
	// -----------------------------

	enum class RefKind : std::uint8_t
	{
		None,
		LValue,
		RValue
	};

	struct TypeRef
	{
		const TypeInfo* base = nullptr;

		bool is_const = false;
		bool is_volatile = false;

		bool is_pointer = false;
		RefKind ref = RefKind::None;

		bool is_array = false;
		std::size_t array_extent = 0; // 0 => unbounded/unknown

		auto operator<=>( const TypeRef& other ) const = default;
	};

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

	using GetFunction = std::function< Instance( Instance ) >;
	using SetFunction = std::function< void( Instance, Instance ) >;

	using ConstructorInvoker = std::function< Instance( std::vector< Instance > ) >;
	using DestructorInvoker = std::function< void( Instance ) >;
	using FunctionInvoker = std::function< Instance( std::vector< Instance > ) >;
	using MethodInvoker = std::function< Instance( Instance, std::vector< Instance > ) >;

	using StreamInsertInvoker = std::function< void( std::ostream&, Instance const& ) >;

	struct PropertyInfo
	{
		std::string_view name;
		const TypeInfo* parent_type;
		TypeRef prop_type;
		GetFunction accessor;
		SetFunction setter;
	};

	struct MethodInfo
	{
		std::string_view name;
		const TypeInfo* parent_type;
		std::optional< TypeRef > return_type; // empty => void
		std::vector< TypeRef > arguments;
		MethodInvoker invoker;
	};

	struct ConstructorInfo
	{
		const TypeInfo* parent_type;
		std::vector< TypeRef > arguments;
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

		RuntimeTypeTraits rttt;

		std::vector< ConstructorInfo > constructors;
		std::optional< DestructorInfo > destructor;
		std::vector< MethodInfo > methods;
		std::vector< PropertyInfo > properties;

		std::optional< StreamInsertInvoker > stream_insert;
	};

	struct FunctionInfo
	{
		std::string_view name;
		std::optional< TypeRef > return_type; // empty => void
		std::vector< TypeRef > arguments;
		FunctionInvoker invoker;
	};

} // namespace sl

namespace std {

	template <>
	struct formatter< sl::Instance, char >
	{
		constexpr auto parse( format_parse_context& ctx )
		{
			auto it = ctx.begin();
			if ( it != ctx.end() && *it != '}' )
				throw format_error( "Invalid format for sl::Instance" );
			return it;
		}

		template < typename FormatContext >
		auto format( sl::Instance const& inst, FormatContext& ctx ) const
		{
			if ( inst.IsVoid() || inst.type == nullptr )
				return format_to( ctx.out(), "<void>" );

			auto const* ti = inst.type;

			if ( ti->stream_insert.has_value() )
			{
				ostringstream oss;
				( *ti->stream_insert )( oss, inst );
				return format_to( ctx.out(), "{}", oss.str() );
			}

			std::string msg;
			msg += "No formatter registered for reflected type: ";
			msg += ti->name;
			throw std::format_error( msg );
		}
	};
} // namespace std
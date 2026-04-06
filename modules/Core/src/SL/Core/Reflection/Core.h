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
		std::size_t size;
		bool is_void;
		bool is_null_pointer;
		bool is_integral;
		bool is_floating_point;
		bool is_member_object_pointer;
		bool is_member_function_pointer;
		bool is_enum;
		bool is_union;
		bool is_class;
		bool is_function;
		bool is_arithmetic;
		bool is_fundamental;
		bool is_object;
		bool is_scalar;
		bool is_compound;
		bool is_member_pointer;
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
		bool is_scoped_enum;

		bool is_default_constructible;
		bool is_copy_constructible;
		bool is_move_constructible;

		template < typename T >
		static RuntimeTypeTraits const* Get()
		{
			using Base = std::remove_cv_t< std::remove_reference_t< T > >;
			static RuntimeTypeTraits traits = Init< Base >();
			return &traits;
		}

	private:
		template < typename T >
		static RuntimeTypeTraits Init()
		{
			static_assert( std::same_as< T, std::remove_cv_t< std::remove_reference_t< T > > > );

			RuntimeTypeTraits traits{};

			traits.size = sizeof( T );
			traits.is_void = std::is_void_v< T >;
			traits.is_null_pointer = std::is_null_pointer_v< T >;
			traits.is_integral = std::is_integral_v< T >;
			traits.is_floating_point = std::is_floating_point_v< T >;
			traits.is_member_object_pointer = std::is_member_object_pointer_v< T >;
			traits.is_member_function_pointer = std::is_member_function_pointer_v< T >;
			traits.is_enum = std::is_enum_v< T >;
			traits.is_union = std::is_union_v< T >;
			traits.is_class = std::is_class_v< T >;
			traits.is_function = std::is_function_v< T >;
			traits.is_arithmetic = std::is_arithmetic_v< T >;
			traits.is_fundamental = std::is_fundamental_v< T >;
			traits.is_object = std::is_object_v< T >;
			traits.is_scalar = std::is_scalar_v< T >;
			traits.is_compound = std::is_compound_v< T >;
			traits.is_member_pointer = std::is_member_pointer_v< T >;
			traits.is_trivial = std::is_trivial_v< T >;
			traits.is_trivially_copyable = std::is_trivially_copyable_v< T >;
			traits.is_standard_layout = std::is_standard_layout_v< T >;
			traits.is_empty = std::is_empty_v< T >;
			traits.is_polymorphic = std::is_polymorphic_v< T >;
			traits.is_abstract = std::is_abstract_v< T >;
			traits.is_final = std::is_final_v< T >;
			traits.is_aggregate = std::is_aggregate_v< T >;
			traits.is_signed = std::is_signed_v< T >;
			traits.is_unsigned = std::is_unsigned_v< T >;
			traits.is_scoped_enum = std::is_scoped_enum_v< T >;

			traits.is_default_constructible = std::is_default_constructible_v< T >;
			traits.is_copy_constructible = std::is_copy_constructible_v< T >;
			traits.is_move_constructible = std::is_move_constructible_v< T >;

			return traits;
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
			{ T::_reflection_data::Info } -> std::same_as< TypeInfo const*& >;
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
		TypeInfo const* base = nullptr;

		bool is_const = false;
		bool is_volatile = false;

		bool is_pointer = false;
		RefKind ref = RefKind::None;

		bool is_array = false;
		std::optional< std::size_t > array_extent{};

		operator bool() const
		{
			return base != nullptr;
		}

		auto operator<=>( const TypeRef& other ) const = default;

		bool IsBase() const
		{
			return base != nullptr and not is_pointer and ref == RefKind::None and not is_array;
		}
	};

	struct Instance
	{
		TypeRef type;
		Any data;

		Instance() = default;

		template < typename T >
		Instance( TypeRef t, T&& d )
			: type( std::move( t ) )
			, data( std::forward< T >( d ) )
		{}

		bool Valid() const
		{
			return data.HasValue();
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

	using StreamInsertInvoker = std::function< void( std::ostream&, void const* ) >;

	struct PropertyInfo
	{
		std::string_view name;
		TypeInfo const* parent_type;
		TypeRef prop_type;
		GetFunction accessor;
		SetFunction setter;
	};

	struct MethodInfo
	{
		std::string_view name;
		TypeInfo const* parent_type;
		std::optional< TypeRef > return_type; // empty => void
		std::vector< TypeRef > arguments;
		MethodInvoker invoker;
	};

	struct ConstructorInfo
	{
		TypeInfo const* parent_type;
		std::vector< TypeRef > arguments;
		ConstructorInvoker invoker;
	};

	struct DestructorInfo
	{
		TypeInfo const* parent_type;
		DestructorInvoker invoker;
	};

	struct TypeInfo
	{
		std::string_view name;

		RuntimeTypeTraits const* rttt;

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

	namespace detail {

		template < typename T >
		using RemoveObjectPointerType =
			std::conditional_t<
				std::is_pointer_v< T > && !std::is_function_v< std::remove_pointer_t< T > >,
				std::remove_pointer_t< T >,
				T >;
	}
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
			if ( inst.IsVoid() || not inst.type )
				return format_to( ctx.out(), "<void>" );

			if ( inst.type.is_array )
				return format_array( inst, ctx );

			if ( inst.type.is_pointer )
				return format_pointer( inst, ctx );

			auto const* ti = inst.type.base;

			if ( ti->stream_insert.has_value() )
			{
				ostringstream oss;
				( *ti->stream_insert )( oss, inst.data.Get< void const* >() );
				return format_to( ctx.out(), "{}", oss.str() );
			}

			std::string msg;
			msg += "No formatter registered for reflected type: ";
			msg += ti->name;
			throw std::format_error( msg );
		}

		template < typename FormatContext >
		auto format_array( sl::Instance const& inst, FormatContext& ctx ) const
		{
			auto const* ti = inst.type.base;

			if ( not inst.type.array_extent.has_value() )
				return format_to( ctx.out(), "<array of {}>", ti->name );

			if ( not ti->stream_insert.has_value() )
				return format_to( ctx.out(), "<array of {}>", ti->name );

			auto raw = reinterpret_cast< std::byte const* >( inst.data.Get< void const* >() );

			std::ostringstream oss;
			oss << "[";

			for ( std::size_t i = 0; i < inst.type.array_extent; ++i )
			{
				if ( i != 0 )
					oss << ", ";

				void const* elem = raw + ( i * ti->rttt->size );
				( *ti->stream_insert )( oss, elem );
			}

			oss << "]";

			return format_to( ctx.out(), "{}", oss.str() );
		}

		template < typename FormatContext >
		auto format_pointer( sl::Instance const& inst, FormatContext& ctx ) const
		{
			auto addr = reinterpret_cast< uintptr_t >( inst.data.Get< void const* >() );
			return format_to( ctx.out(), "{:#0{}x}", addr, 2 + sizeof( std::uintptr_t ) * 2 );
		}
	};
} // namespace std
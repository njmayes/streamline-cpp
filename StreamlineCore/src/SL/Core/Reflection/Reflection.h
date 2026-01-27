#pragma once

#include "Core.h"
#include "Exception.h"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace sl::reflect {

	class Reflection
	{
	public:
		template < typename T >
		static const TypeInfo* GetInfo()
		{
			using NoRef = std::remove_reference_t< T >;
			using NoPtr = std::remove_pointer_t< NoRef >;
			using Base = std::remove_cv_t< NoPtr >;

			using Traits = TypeTraits< Base >;

			if ( not sReflectionData.contains( Traits::Name ) )
				Register< Base >();

			return &sReflectionData[ Traits::Name ];
		}

		static const TypeInfo* GetInfo( std::string_view name )
		{
			if ( not sReflectionData.contains( name ) )
				throw std::runtime_error( "Attempting to get reflection type by name that didn't exist." );

			return &sReflectionData[ name ];
		}

		template < typename T, typename... Args >
			requires std::is_constructible_v< T, Args... >
		static void RegisterConstructor( detail::Ctr< T, Args... > )
		{
			auto type_info = GetInfoForAddition< T >();

			using Params = TypeList< Args... >;

			ConstructorInfo ctr;
			ctr.parent_type = type_info;
			ctr.arguments = { MakeTypeRef< Args >()... };

			ctr.invoker = []( std::vector< Instance > instanced_args = {} ) -> Instance {
				auto generate_tuple_val = []< std::size_t I >( Instance& object ) -> decltype( auto ) {
					using ArgType = typename Params::template Type< I >;
					return object.data.template Get< ArgType >();
				};

				auto generate_tuple = [ & ]< std::size_t... Is >( std::vector< Instance >& args, std::index_sequence< Is... > ) -> Params::TupleType {
					return typename Params::TupleType { generate_tuple_val.template operator()< Is >( args[ Is ] )... };
				};

				auto tuple_params = generate_tuple( instanced_args, std::make_index_sequence< Params::Size >() );

				auto ctr_func = []< typename... U >( U&&... u ) {
					return T( std::forward< U >( u )... );
				};

				return Instance(
					GetInfo< T >(),
					std::apply( ctr_func, std::move( tuple_params ) )
				);
			};

			type_info->constructors.push_back( std::move( ctr ) );
		}

		template < typename T, typename... Args >
		static void RegisterDestructor()
		{
			auto* type_info = GetInfoForAddition< T >();

			DestructorInfo dtr;
			dtr.parent_type = type_info;
			dtr.invoker = []( Instance object ) {
				if ( object.type->name != TypeTraits< std::remove_cvref_t< T > >::Name )
					return;
				object.data.Get< T& >().~T();
			};

			type_info->destructor.emplace( std::move( dtr ) );
		}

		template < typename T, typename MemberType >
		static void RegisterMember( std::string_view name, MemberType accessor )
		{
			if constexpr ( std::is_member_object_pointer_v< MemberType > )
				RegisterProperty< T, MemberType >( name, accessor );
			else if constexpr ( std::is_member_function_pointer_v< MemberType > )
				RegisterMethod< T, MemberType >( name, accessor );
		}

	private:
		template < typename T >
		static TypeRef MakeTypeRef()
		{
			using NoRef = std::remove_reference_t< T >;
			using NoPtr = std::remove_pointer_t< NoRef >;
			using Base = std::remove_cv_t< NoPtr >;

			TypeRef out{};
			out.base = GetInfo< Base >();

			out.is_pointer = std::is_pointer_v< NoRef >;

			if constexpr ( std::is_lvalue_reference_v< T > )
				out.ref = RefKind::LValue;
			else if constexpr ( std::is_rvalue_reference_v< T > )
				out.ref = RefKind::RValue;
			else
				out.ref = RefKind::None;

			out.is_const = std::is_const_v< NoPtr >;
			out.is_volatile = std::is_volatile_v< NoPtr >;

			if constexpr ( std::is_array_v< NoRef > )
			{
				out.is_array = true;
				if constexpr ( std::is_bounded_array_v< NoRef > )
					out.array_extent = std::extent_v< NoRef >;
			}

			return out;
		}

		template < typename T >
		static TypeInfo* GetInfoForAddition()
		{
			using Traits = TypeTraits< std::remove_cvref_t< T > >;

			if ( not sReflectionData.contains( Traits::Name ) )
				Register< std::remove_cvref_t< T > >();

			return &sReflectionData[ Traits::Name ];
		}

		template < typename T >
		static void Register()
		{
			using Traits = TypeTraits< T >;

			TypeInfo new_type;
			new_type.name = Traits::Name;
			new_type.rttt.Init< T >();

			sReflectionData.emplace( Traits::Name, std::move( new_type ) );

			if constexpr ( detail::IsReflectableType< T > )
			{
				if constexpr ( std::is_default_constructible_v< T > )
					RegisterConstructor( detail::Ctr< T >{} );
				if constexpr ( std::is_copy_constructible_v< T > )
					RegisterConstructor( detail::Ctr< T, const T& >{} );
				if constexpr ( std::is_move_constructible_v< T > )
					RegisterConstructor( detail::Ctr< T, T&& >{} );

				if constexpr ( std::is_destructible_v< T > )
					RegisterDestructor< T >();

				T::_reflection_data::Build();
			}

			if constexpr ( requires( std::ostream& os, T const& v ) { os << v; } )
			{
				RegisterStreamInsert< T >();
			}
		}

		template < typename T, typename MemberType >
		static void RegisterProperty( std::string_view name, MemberType accessor )
		{
			using PropType = typename PropertyTraits< decltype( accessor ) >::PropType;

			auto* type_info = GetInfoForAddition< T >();

			PropertyInfo prop;
			prop.name = name;
			prop.parent_type = type_info;

			prop.prop_type = MakeTypeRef< PropType >();

			prop.accessor = [ accessor ]( Instance ctx ) {
				return Instance(
					GetInfo< PropType >(),
					ctx.data.Get< const T& >().*accessor
				);
			};

			prop.setter = [ accessor ]( Instance ctx, Instance value ) {
				ctx.data.Get< T& >().*accessor = value.data.Get< PropType >();
			};

			type_info->properties.push_back( std::move( prop ) );
		}

		template < typename T, typename MemberType >
		static void RegisterMethod( std::string_view name, MemberType accessor )
		{
			auto* type_info = GetInfoForAddition< T >();

			using MemberTraits = FunctionTraits< MemberType >;
			using ArgTypes = typename MemberTraits::Arguments;
			using ReturnType = typename MemberTraits::ReturnType;

			SCONSTEXPR bool IsReturnVoid = std::same_as< ReturnType, void >;

			auto get_arg_types = []< std::size_t... Is >( std::index_sequence< Is... > ) -> std::vector< TypeRef > {
				return { MakeTypeRef< typename ArgTypes::template Type< Is > >()... };
			};

			MethodInfo method;
			method.name = name;
			method.parent_type = type_info;
			method.arguments = get_arg_types( std::make_index_sequence< ArgTypes::Size >() );
			if constexpr ( not IsReturnVoid )
				method.return_type = MakeTypeRef< ReturnType >();

			method.invoker = [ accessor ]( Instance ctx, std::vector< Instance > args = {} ) -> Instance {
				auto generate_tuple_val = []< std::size_t I >( Instance& object ) {
					using ArgType = typename ArgTypes::template Type< I >;
					return object.data.Get< ArgType >();
				};

				auto generate_tuple = [ & ]< std::size_t... Is >( std::vector< Instance >& argv, std::index_sequence< Is... > ) {
					return std::make_tuple( generate_tuple_val.template operator()< Is >( argv[ Is ] )... );
				};

				auto tuple_params = generate_tuple( args, std::make_index_sequence< ArgTypes::Size >() );

				if constexpr ( IsReturnVoid )
				{
					auto func = [ ctx, accessor ]< typename... Args >( Args&&... argv ) mutable {
						auto& ctx_ref = ctx.data.Get< T& >();
						( ctx_ref.*accessor )( std::forward< Args >( argv )... );
					};
					std::apply( func, tuple_params );
					return {};
				}
				else
				{
					auto func = [ ctx, accessor ]< typename... Args >( Args&&... argv ) mutable -> ReturnType {
						auto& ctx_ref = ctx.data.Get< T& >();
						return ( ctx_ref.*accessor )( std::forward< Args >( argv )... );
					};

					return Instance(
						GetInfo< ReturnType >(),
						std::apply( func, std::move( tuple_params ) )
					);
				}
			};

			type_info->methods.push_back( std::move( method ) );
		}

		template < typename T >
		static void RegisterStreamInsert()
		{
			static_assert(
				requires( std::ostream& os, T const& v ) { os << v; },
				"RegisterStreamInsert<T> requires: std::ostream& operator<<(std::ostream&, T const&)"
			);

			auto* type_info = GetInfoForAddition< T >();

			type_info->stream_insert = []( std::ostream& os, Instance const& value ) {
				if constexpr ( std::is_copy_constructible_v< T > )
				{
					os << value.data.template Get< T >();
				}
				else
				{
					os << value.data.template Get< T const& >();
				}
			};
		}

	private:
		using ReflectionData = std::unordered_map< std::string_view, TypeInfo >;
		inline static ReflectionData sReflectionData;
	};

	template < typename T >
	Instance MakeInstance( T&& value )
	{
		return Instance(
			reflect::Reflection::GetInfo< T >(),
			std::forward< T >( value )
		);
	}

} // namespace sl::reflect

#define SLC_REFLECT_MEMBER_IMPL( member )                                                              \
	{                                                                                                  \
		auto invoker = []< typename _refl > {                                                          \
			if constexpr ( std::derived_from< _refl, ::sl::detail::CtrBase > )                         \
				::sl::reflect::Reflection::RegisterConstructor< ClassType >( _refl{} );                \
			else                                                                                       \
				::sl::reflect::Reflection::RegisterMember< ClassType >( #member, &ClassType::member ); \
		};                                                                                             \
		using MemberType = decltype( &ClassType::member );                                             \
		invoker.template operator()< MemberType >();                                                   \
	}

#define SLC_REFLECT_CLASS( CLASS, ... )                                                     \
	using Reflectable< CLASS >::Ctr;                                                        \
	using Reflectable< CLASS >::ArgumentType;                                               \
	struct _reflection_data                                                                 \
	{                                                                                       \
		static void Build()                                                                 \
		{                                                                                   \
			using ClassType = CLASS;                                                        \
			SLC_FOR_EACH( SLC_REFLECT_MEMBER_IMPL, __VA_ARGS__ )                            \
		}                                                                                   \
		inline static const TypeInfo* Info = ::sl::reflect::Reflection::GetInfo< CLASS >(); \
	};

#define SLC_REMOVE_PAREN( ... ) ArgumentType< void( __VA_ARGS__ ) >::type

#define SLC_CTR( ... )                              \
	template SLC_REMOVE_PAREN( Ctr< __VA_ARGS__ > ) \
	{}

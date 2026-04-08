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

namespace sl {
	class Type;
}

namespace sl::reflect {

	class Reflection
	{
	public:
		template < typename T >
		static const TypeInfo* GetInfo()
		{
			using NoRef = std::remove_reference_t< T >;
			using NoPtr = detail::RemoveObjectPointerType< NoRef >;
			using NoArr = std::remove_extent_t< NoPtr >;
			using Base = std::remove_cv_t< NoArr >;

			using Traits = TypeTraits< Base >;

			if ( not sReflectedTypes.contains( Traits::Name ) )
				Register< Base >();

			return &sReflectedTypes[ Traits::Name ];
		}

		static const TypeInfo* GetInfo( std::string_view name )
		{
			if ( not sReflectedTypes.contains( name ) )
				throw std::runtime_error( "Attempting to get reflection type by name that didn't exist." );

			return &sReflectedTypes[ name ];
		}

		template < typename T >
		static TypeRef GetTypeRef()
		{
			return MakeTypeRef< T >();
		}

		static TypeRef GetTypeRef( std::string_view name )
		{
			return TypeRefFromString( name );
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
					using ArgType = typename Params::template TypeAt< I >;
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
					GetTypeRef< T >(),
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
				if ( object.type.base->name != TypeTraits< std::remove_cvref_t< T > >::Name )
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
			using NoPtr = detail::RemoveObjectPointerType< NoRef >;
			using NoArr = std::remove_extent_t< NoPtr >;
			using Base = std::remove_cv_t< NoArr >;

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

			if ( not sReflectedTypes.contains( Traits::Name ) )
				Register< std::remove_cvref_t< T > >();

			return &sReflectedTypes[ Traits::Name ];
		}

		template < typename T >
		static void Register()
		{
			using Traits = TypeTraits< T >;

			TypeInfo new_type;
			new_type.name = Traits::Name;
			new_type.rttt = RuntimeTypeTraits::Get< T >();

			sReflectedTypes.emplace( Traits::Name, std::move( new_type ) );

			if constexpr ( std::is_default_constructible_v< T > )
				RegisterConstructor( detail::Ctr< T >{} );
			if constexpr ( std::is_copy_constructible_v< T > )
				RegisterConstructor( detail::Ctr< T, const T& >{} );
			if constexpr ( std::is_move_constructible_v< T > )
				RegisterConstructor( detail::Ctr< T, T&& >{} );

			if constexpr ( std::is_destructible_v< T > )
				RegisterDestructor< T >();

			if constexpr ( requires( std::ostream& os, T const& v ) { os << v; } )
				RegisterStreamInsert< T >();

			if constexpr ( detail::IsReflectableType< T > )
			{
				if constexpr ( detail::IsReflectableType< T > )
				{
					T::_reflection_data::Build();
				}
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
					GetTypeRef< PropType >(),
					ctx.data.Get< const T& >().*accessor
				);
			};

			if constexpr ( std::is_assignable_v< PropType&, PropType > )
			{
				prop.setter = [ accessor ]( Instance ctx, Instance value ) {
					ctx.data.Get< T& >().*accessor = value.data.Get< PropType >();
				};
			}

			type_info->properties.push_back( std::move( prop ) );
		}

		template < typename T, typename MemberType >
		static void RegisterMethod( std::string_view name, MemberType accessor )
		{
			auto* type_info = GetInfoForAddition< T >();

			using MemberTraits = FunctionTraits< MemberType >;
			using ArgTypes = typename MemberTraits::Arguments;
			using ReturnType = typename MemberTraits::ReturnType;

			static constexpr bool IsReturnVoid = std::same_as< ReturnType, void >;

			auto get_arg_types = []< std::size_t... Is >( std::index_sequence< Is... > ) -> std::vector< TypeRef > {
				return { MakeTypeRef< typename ArgTypes::template TypeAt< Is > >()... };
			};

			MethodInfo method;
			method.name = name;
			method.parent_type = type_info;
			method.arguments = get_arg_types( std::make_index_sequence< ArgTypes::Size >() );
			if constexpr ( not IsReturnVoid )
				method.return_type = MakeTypeRef< ReturnType >();

			method.invoker = [ accessor ]( Instance ctx, std::vector< Instance > args = {} ) -> Instance {
				auto generate_tuple_val = []< std::size_t I >( Instance& object ) {
					using ArgType = typename ArgTypes::template TypeAt< I >;
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

			type_info->stream_insert = []( std::ostream& os, void const* ptr ) {
				os << *static_cast< T const* >( ptr );
			};
		}

		static std::string TypeRefToString( TypeRef const& tr )
		{
			return std::format(
				"{}{}{}{}{}{}",
				tr.is_const ? "const " : "",
				tr.is_volatile ? "volatile " : "",
				tr.base ? tr.base->name : "<invalid type>",
				tr.is_pointer ? "*" : "",
				tr.ref == RefKind::LValue ? "&" : ( tr.ref == RefKind::RValue ? "&&" : "" ),
				tr.is_array ? "[" + ( tr.array_extent.has_value() ? std::to_string( tr.array_extent.value() ) : "" ) + "]" : ""
			);
		}


		static TypeRef TypeRefFromString( std::string_view text )
		{
			auto trim = []( std::string_view sv ) -> std::string_view {
				while ( !sv.empty() && std::isspace( static_cast< unsigned char >( sv.front() ) ) )
					sv.remove_prefix( 1 );
				while ( !sv.empty() && std::isspace( static_cast< unsigned char >( sv.back() ) ) )
					sv.remove_suffix( 1 );
				return sv;
			};

			auto starts_with_word = [ & ]( std::string_view sv, std::string_view word ) -> bool {
				if ( sv.size() < word.size() )
					return false;
				if ( sv.substr( 0, word.size() ) != word )
					return false;
				if ( sv.size() == word.size() )
					return true;

				char next = sv[ word.size() ];
				return std::isspace( static_cast< unsigned char >( next ) ) != 0;
			};

			auto consume_prefix_word = [ & ]( std::string_view& sv, std::string_view word ) -> bool {
				sv = trim( sv );
				if ( !starts_with_word( sv, word ) )
					return false;

				sv.remove_prefix( word.size() );
				sv = trim( sv );
				return true;
			};

			auto consume_suffix = [ & ]( std::string_view& sv, std::string_view suffix ) -> bool {
				sv = trim( sv );
				if ( sv.size() < suffix.size() )
					return false;
				if ( sv.substr( sv.size() - suffix.size() ) != suffix )
					return false;

				sv.remove_suffix( suffix.size() );
				sv = trim( sv );
				return true;
			};

			auto parse_array_suffix = [ & ]( std::string_view& sv, bool& is_array, std::optional< std::size_t >& extent ) -> bool {
				sv = trim( sv );

				if ( sv.empty() || sv.back() != ']' )
					return false;

				auto open = sv.find_last_of( '[' );
				if ( open == std::string_view::npos || open > sv.size() - 1 )
					throw std::runtime_error( "Invalid array suffix in type name." );

				std::string_view inside = sv.substr( open + 1, sv.size() - open - 2 );
				inside = trim( inside );

				is_array = true;
				if ( not inside.empty() )
				{
					std::size_t value = 0;
					for ( char ch : inside )
					{
						if ( ch < '0' || ch > '9' )
							throw std::runtime_error( "Invalid array extent in type name." );
						value = ( value * 10 ) + static_cast< std::size_t >( ch - '0' );
					}
					extent = value;
				}

				sv = sv.substr( 0, open );
				sv = trim( sv );
				return true;
			};

			text = trim( text );
			if ( text.empty() )
				throw std::runtime_error( "Cannot parse empty type name." );

			TypeRef out{};

			// Parse suffixes in reverse order of ToString():
			//   const volatile Base*[N]&&
			//                         ^ ref
			//                      ^^^^^^^ array
			//                    ^ pointer

			if ( consume_suffix( text, "&&" ) )
				out.ref = RefKind::RValue;
			else if ( consume_suffix( text, "&" ) )
				out.ref = RefKind::LValue;

			parse_array_suffix( text, out.is_array, out.array_extent );

			if ( consume_suffix( text, "*" ) )
				out.is_pointer = true;

			// Parse prefixes in the same order ToString() emits them.
			if ( consume_prefix_word( text, "const" ) )
				out.is_const = true;

			if ( consume_prefix_word( text, "volatile" ) )
				out.is_volatile = true;

			text = trim( text );
			if ( text.empty() )
				throw std::runtime_error( "Missing base type name in type string." );

			out.base = Reflection::GetInfo( text );
			return out;
		}

	private:
		using ReflectionData = std::unordered_map< std::string_view, TypeInfo >;
		inline static ReflectionData sReflectedTypes;

		friend Type;
	};

	template < typename T >
	Instance MakeInstance( T&& value )
	{
		return Instance(
			Reflection::GetTypeRef< T >(),
			std::forward< T >( value )
		);
	}

} // namespace sl::reflect

#define SL_REFLECT_MEMBER_IMPL( member )                                                               \
	{                                                                                                  \
		auto invoker = []< typename _refl > {                                                          \
			if constexpr ( ::sl::DerivedFromOnly< _refl, ::sl::detail::CtrBase > )                     \
				::sl::reflect::Reflection::RegisterConstructor< ClassType >( _refl{} );                \
			else                                                                                       \
				::sl::reflect::Reflection::RegisterMember< ClassType >( #member, &ClassType::member ); \
		};                                                                                             \
		using MemberType = decltype( &ClassType::member );                                             \
		invoker.template operator()< MemberType >();                                                   \
	}

#define SL_REFLECT_CLASS( CLASS, ... )                                                            \
	template < typename... Args >                                                                 \
	using _ctr = ::sl::detail::Ctr< CLASS, Args... >;                                             \
                                                                                                  \
	template < typename _R >                                                                      \
	struct _arg_type;                                                                             \
	template < typename _R, typename _U >                                                         \
	struct _arg_type< _R( _U ) >                                                                  \
	{                                                                                             \
		using type = _U;                                                                          \
	};                                                                                            \
                                                                                                  \
	struct _reflection_data                                                                       \
	{                                                                                             \
		static void Build()                                                                       \
		{                                                                                         \
			using ClassType = CLASS;                                                              \
			SL_FOR_EACH( SL_REFLECT_MEMBER_IMPL, __VA_ARGS__ )                                    \
		}                                                                                         \
		inline static const ::sl::TypeInfo* Info = ::sl::reflect::Reflection::GetInfo< CLASS >(); \
	};                                                                                            \
                                                                                                  \
	std::string ToString() const                                                                  \
	{                                                                                             \
		return ::sl::Type::Get< CLASS >().ToString( *this );                                      \
	}

#define SL_REMOVE_PAREN( ... ) _arg_type< void( __VA_ARGS__ ) >::type

#define SL_CTR( ... )                               \
	template SL_REMOVE_PAREN( _ctr< __VA_ARGS__ > ) \
	{}

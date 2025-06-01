#pragma once

#include "Core.h"
#include "Exception.h"

namespace slc::reflect {

	class Reflection
	{
	public:
		template < typename T >
		static const TypeInfo* GetInfo()
		{
			using Traits = TypeTraits< T >;

			if ( not sReflectionData.contains( Traits::Name ) )
				Register< T >();

			return &sReflectionData[ Traits::Name ];
		}

		template < typename T, typename... Args >
			requires std::is_constructible_v< T, Args... >
		static void RegisterConstructor( detail::Ctr< T, Args... > )
		{
			auto type_info = GetInfoForAddition< T >();

			using Params = TypeList< Args... >;

			ConstructorInfo ctr;
			ctr.parent_type = type_info;
			ctr.arguments = { GetInfo< Args >()... };

			ctr.invoker = []( std::vector< Instance > instanced_args = {} ) {
				auto gen_tuple_val = []< std::size_t I >( Instance object ) {
					using ArgType = typename Params::template Type< I >;
					return object.data.Get< ArgType >();
				};

				auto gen_tuple = []< std::size_t... Is >( std::vector< Instance > args, std::index_sequence< Is... > ) -> Params::TupleType {
					return std::make_tuple( gen_tuple_val.template operator()< Is >( std::move( args[ Is ] ) )... );
				};

				auto tuple_params = gen_tuple( std::move( instanced_args ), std::make_index_sequence< Params::Size >() );

				auto ctr_func = []( Args&&... args ) {
					return T( std::forward< Args >( args )... );
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
				if ( object.type->name != TypeTraits< T >::Name )
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
		static TypeInfo* GetInfoForAddition()
		{
			using Traits = TypeTraits< T >;

			if ( not sReflectionData.contains( Traits::Name ) )
				Register< T >();

			return &sReflectionData[ Traits::Name ];
		}

		template < typename T >
		static void Register()
		{
			using Traits = TypeTraits< T >;

			TypeInfo new_type;
			new_type.name = Traits::Name;
			new_type.base_name = Traits::BaseName;
			new_type.rttt.Init< T >();

			sReflectionData.emplace( Traits::Name, std::move( new_type ) );

			SCONSTEXPR bool IsReflectableType = std::derived_from< T, Reflectable< T > >;
			if constexpr ( IsReflectableType )
			{
				RegisterBaseClasses< T >( BaseClassList< T >{} );

				if constexpr ( std::is_default_constructible_v< T > )
					RegisterConstructor( detail::Ctr< T >{} );
				if constexpr ( std::is_copy_constructible_v< T > )
					RegisterConstructor( detail::Ctr< T, const T& >{} );
				if constexpr ( std::is_move_constructible_v< T > )
					RegisterConstructor( detail::Ctr< T, T&& >{} );

				if constexpr ( std::is_destructible_v< T > )
					RegisterDestructor< T >();

				T::slc_refl_data::Build();
			}
		}

		template < typename T, typename... Ts >
		static void RegisterBaseClasses( TypeList< Ts... > )
		{
			auto* type_info = GetInfoForAddition< T >();

			( [ = ]() {
				using BaseType = typename Ts::type;
				if constexpr ( not std::same_as< T, BaseType > and CanReflect< BaseType > )
				{
					type_info->base_types.push_back( GetInfo< BaseType >() );
				}
			}(),
			  ... );
		}

		template < typename T, typename MemberType >
		static void RegisterProperty( std::string_view name, MemberType accessor )
		{
			using PropType = typename PropertyTraits< decltype( accessor ) >::PropType;

			auto* type_info = GetInfoForAddition< T >();

			PropertyInfo prop;
			prop.name = name;
			prop.parent_type = type_info;

			prop.prop_type = GetInfo< PropType >();

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
			using ArgTypes = MemberTraits::Arguments;
			using ReturnType = typename MemberTraits::ReturnType;

			SCONSTEXPR bool IsReturnVoid = std::same_as< ReturnType, void >;

			auto get_arg_types = []< std::size_t... Is >( std::index_sequence< Is... > ) -> std::vector< const TypeInfo* > {
				return { GetInfo< typename ArgTypes::template Type< Is > >()... };
			};

			auto arg_types = get_arg_types( std::make_index_sequence< ArgTypes::Size >() );

			MethodInfo method;
			method.name = name;
			method.parent_type = type_info;
			method.arguments = std::move( arg_types );
			if constexpr ( not IsReturnVoid )
				method.return_type = GetInfo< ReturnType >();

			method.invoker = []( Instance ctx, std::vector< Instance > args = {} ) -> Instance {
				auto gen_tuple_val = []< std::size_t I >( Instance& object ) {
					using ArgType = ArgTypes::template Type< I >;
					return object.data.Get< ArgType >();
				};

				auto gen_tuple = [ & ]< std::size_t... Is >( std::vector< Instance >& args, std::index_sequence< Is... > ) {
					return std::make_tuple( gen_tuple_val.template operator()< Is >( args[ Is ] )... );
				};

				auto tuple_params = gen_tuple( args, std::make_index_sequence< ArgTypes::Size >() );

				if constexpr ( IsReturnVoid )
				{
					auto func = [ ctx ]< typename... Args >( Args&&... argv ) {
						auto& ctx_ref = ctx.data.Get< T& >();
						( ctx_ref.*accessor )( std::forward< Args >( argv )... );
					};
					std::apply( func, tuple_params );
					return {};
				}
				else
				{
					auto func = [ ctx ]< typename... Args >( Args&&... argv ) -> ReturnType {
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

	private:
		using ReflectionData = std::unordered_map< std::string_view, TypeInfo >;
		inline static ReflectionData sReflectionData;
	};
} // namespace slc::reflect

#define SLC_REFLECT_MEMBER_IMPL( member )                                                               \
	{                                                                                                   \
		auto invoker = []< typename _T > {                                                              \
			if constexpr ( std::derived_from< _T, ::slc::detail::CtrBase > )                            \
				::slc::reflect::Reflection::RegisterConstructor< ClassType >( _T{} );                   \
			else                                                                                        \
				::slc::reflect::Reflection::RegisterMember< ClassType >( #member, &ClassType::member ); \
		};                                                                                              \
		using MemberType = decltype( &ClassType::member );                                              \
		invoker.template operator()< MemberType >();                                                    \
	}

#define SLC_REFLECT_CLASS( CLASS, ... )                                                      \
	using Reflectable< CLASS >::Ctr;                                                         \
	using Reflectable< CLASS >::ArgumentType;                                                \
	struct slc_refl_data                                                                     \
	{                                                                                        \
		static void Build()                                                                  \
		{                                                                                    \
			using ClassType = CLASS;                                                         \
			SLC_FOR_EACH( SLC_REFLECT_MEMBER_IMPL, __VA_ARGS__ )                             \
		}                                                                                    \
		inline static const TypeInfo* Info = ::slc::reflect::Reflection::GetInfo< CLASS >(); \
	};


#define SLC_REMOVE_PAREN( ... ) ArgumentType< void( __VA_ARGS__ ) >::type

#define SLC_CTR( ... )                              \
	template SLC_REMOVE_PAREN( Ctr< __VA_ARGS__ > ) \
	{}

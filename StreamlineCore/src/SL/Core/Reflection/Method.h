#pragma once

#include "Reflection.h"

namespace sl {

	class Type;

	class Method
	{
	public:
		Method() = default;
		Method( const MethodInfo& info )
			: mMethod( &info )
		{}

		std::string_view GetName() const
		{
			return mMethod->name;
		}

		Type GetReturnType() const;
		std::vector< Type > GetArgumentTypes() const;

		Instance Invoke( Instance obj, std::vector< Instance >& args ) const
		{
			return mMethod->invoker( std::move( obj ), args );
		}

		template < typename T, CanReflect Obj, CanReflect... Args >
		T Invoke( Obj&& obj, Args&&... args ) const
		{
			using ReturnTraits = TypeTraits< std::remove_cvref_t< T > >;
			using ObjTraits = TypeTraits< std::remove_cvref_t< Obj > >;

			// Return type check: compare canonical base type names.
			if constexpr ( not std::is_void_v< T > )
			{
				if ( not mMethod->return_type.has_value() )
					throw BadReflectionCastException( ReturnTraits::Name, "void" );

				if ( mMethod->return_type->base->name != ReturnTraits::Name )
					throw BadReflectionCastException( ReturnTraits::Name, mMethod->return_type->base->name );
			}

			// Parent object type check (canonical)
			if ( mMethod->parent_type->name != ObjTraits::Name )
				throw BadReflectionCastException( ObjTraits::Name, mMethod->parent_type->name );

			if ( sizeof...( Args ) != mMethod->arguments.size() )
				throw std::invalid_argument( "Number of arguments provided does not match the number of arguments of the method." );

			auto make_instance_arg = [ & ]< typename R >( R&& value ) -> Instance {
				return reflect::MakeInstance( std::forward< R >( value ) );
			};

			std::vector< Instance > instanced_args = { make_instance_arg( std::forward< Args >( args ) )... };

			auto result = mMethod->invoker(
				reflect::MakeInstance( std::forward< Obj >( obj ) ),
				std::move( instanced_args )
			);

			if constexpr ( std::is_void_v< T > )
				return;
			else
				return result.data.template Get< T >();
		}

	private:
		const MethodInfo* mMethod = nullptr;
	};
} // namespace sl

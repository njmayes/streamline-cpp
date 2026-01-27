#pragma once

#include "Reflection.h"

namespace sl {

	class Type;

	class Property
	{
	public:
		Property() = default;
		Property( const PropertyInfo& info )
			: mProperty( &info )
		{}

		Type GetType() const;

		std::string_view GetName() const
		{
			return mProperty->name;
		}

		template < CanReflect T, CanReflect Obj >
		const T& GetValue( const Obj& obj ) const
		{
			using Traits = TypeTraits< std::remove_cvref_t< T > >;
			using ObjTraits = TypeTraits< std::remove_cvref_t< Obj > >;

			// Property stores TypeRef; compare canonical base type.
			if ( mProperty->prop_type.base->name != Traits::Name )
				throw BadReflectionCastException( Traits::Name, mProperty->prop_type.base->name );

			if ( mProperty->parent_type->name != ObjTraits::Name )
				throw BadReflectionCastException( ObjTraits::Name, mProperty->parent_type->name );

			auto instance = GetValue( obj );
			return instance.data.template Get< const T& >();
		}

		template < CanReflect Obj >
		Instance GetValue( const Obj& obj ) const
		{
			return mProperty->accessor( reflect::MakeInstance( obj ) );
		}

		template < CanReflect T, CanReflect Obj >
		void SetValue( Obj& obj, const T& value )
		{
			using Traits = TypeTraits< std::remove_cvref_t< T > >;

			if ( mProperty->prop_type.base->name != Traits::Name )
				throw BadReflectionCastException( Traits::Name, mProperty->prop_type.base->name );

			SetValue( obj, reflect::MakeInstance( value ) );
		}

		template < CanReflect Obj >
		void SetValue( Obj& obj, Instance value )
		{
			return mProperty->setter( reflect::MakeInstance( obj ), std::move( value ) );
		}

	private:
		const PropertyInfo* mProperty = nullptr;
	};
} // namespace sl

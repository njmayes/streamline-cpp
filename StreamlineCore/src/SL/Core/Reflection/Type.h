#pragma once

#include "Property.h"
#include "Method.h"

namespace slc {

	/// <summary>
	/// The main interface for runtime reflection. Retrieve the reflection data for a type T using Type::Get&lt;T&gt;() or Type::Get("T")
	///
	/// There are two components to using a class T for reflection.
	///
	/// The first is to inherit from Reflectable&lt;T&gt;, which provides the minimum data in order to register a type in the reflection database,
	/// such as base classes, type traits, as well as default, copy, and move constructors and the destructor where applicable.
	///
	/// In order to reflect further object data, such as class members (data and functions) and additional constructors, use the SLC_REFLECT_CLASS
	/// macro in the class scope, listing constructors using the SLC_CTR macro, and members using their unqualified name.
	/// </summary>
	///
	/// <example>
	/// struct Foo : Reflectable&lt;Foo&gt;
	/// {
	///		Foo(double, int) {}
	///
	///		int bar;
	///		void baz() {}
	///
	///		SLC_REFLECT_CLASS(Foo,
	///			SLC_CTR(double, int),
	///			bar, baz
	///		)
	/// };
	/// </example>
	class Type
	{
	public:
		Type( const TypeInfo* type )
			: mInfo( type )
		{}

		std::string_view GetName() const
		{
			return mInfo->name;
		}

		template < CanReflect T, typename... Args >
			requires std::constructible_from< T, Args... >
		T Instantiate( Args&&... args ) const
		{
			if ( mInfo->name != TypeTraits< T >::Name )
				throw BadReflectionCastException( TypeTraits< T >::Name, mInfo->name );

			auto ctr = FindConstructor< Args&&... >();
			if ( not ctr )
				throw UnreflectedTargetException< Args... >( mInfo->name );

			std::vector< Instance > instanced_args;
			instanced_args.reserve( sizeof...( Args ) );

			( [ & ]() {
				instanced_args.emplace_back( MakeInstance( std::forward< Args >( args ) ) );
			}(),
			  ... );

			auto instance = ctr->invoker( std::move( instanced_args ) );
			return instance.data.template Get< T >();
		}

		template < typename T, CanReflect Obj, typename... Args >
		T InvokeMember( std::string_view name, Obj&& obj, Args&&... args ) const
		{
			return GetMethod( name ).Invoke< T >( std::forward< Obj >( obj ), std::forward< Args >( args )... );
		}

		std::vector< Type > GetBaseClasses() const;

		Property GetProperty( std::string_view name ) const;
		std::vector< Property > GetProperties() const;

		Method GetMethod( std::string_view name ) const;
		std::vector< Method > GetMethods() const;

		template < CanReflect T >
		bool IsSubclassOf() const
		{
			return IsSubclassOf( T::ReflectionData::Info );
		}
		bool IsSubclassOf( const Type& other ) const;

		auto operator<=>( const Type& ) const = default;
		operator bool() const
		{
			return mInfo;
		}

	public:
		template < CanReflect T >
		static Type Get()
		{
			if constexpr ( detail::IsReflectableType< T > )
			{
				return Type( T::slc_refl_data::Info );
			}
			else
			{
				return Type( reflect::Reflection::GetInfo< T >() );
			}
		}

	private:
		template < typename... Args >
		const ConstructorInfo* FindConstructor() const
		{
			using ArgTypes = TypeList< Args... >;

			auto is_covertible = [ & ]< std::size_t I >( const TypeInfo* ctr_arg ) {
				using ArgType = ArgTypes::template Type< I >;
				return IsConvertibleTo< ArgType >( ctr_arg );
			};

			auto match_param = [ & ]< std::size_t... Is >( const ConstructorInfo& ctr, std::index_sequence< Is... > ) {
				return ( ... and is_covertible.template operator()< Is >( ctr.arguments[ Is ] ) );
			};

			auto match_parameters = [ & ]( const ConstructorInfo& ctr ) {
				if ( ctr.arguments.size() != ArgTypes::Size )
					return false;

				return match_param( ctr, std::make_index_sequence< ArgTypes::Size >() );
			};

			auto it = std::ranges::find_if( mInfo->constructors, match_parameters );
			return it != mInfo->constructors.end() ? &( *it ) : nullptr;
		}

	private:
		const TypeInfo* mInfo;

		template < typename Arg >
		static bool IsConvertibleTo( const TypeInfo* target )
		{
			using Traits = TypeTraits< Arg >;

			SLC_TODO( "Support conversions between types, not just between value categories of same type" );
			if ( Traits::BaseName != target->base_name )
				return false;

			if constexpr ( Traits::IsLValueReference )
			{
				if constexpr ( Traits::IsConst )
				{
					// const& parameter can be used for const& argument as well as for lvalue arguments of types with copy constructor
					return ( target->rttt.is_const and target->rttt.is_lvalue_reference ) or
						   ( not target->rttt.is_reference and target->rttt.is_copy_constructible );
				}
				else
				{
					//& parameter can be used for & argument as well as for lvalue arguments of types with copy constructor
					return ( target->rttt.is_lvalue_reference ) or
						   ( not target->rttt.is_reference and target->rttt.is_copy_constructible );
				}
			}
			else if constexpr ( Traits::IsRValueReference )
			{
				//&& parameter can be used for && argument or lvalue arguments with move constructor
				return ( target->rttt.is_rvalue_reference ) or
					   ( not target->rttt.is_reference and target->rttt.is_move_constructible );
			}
			else
			{
				// Value paramater can be used for any argument type (except rvalue ref)
				return not target->rttt.is_rvalue_reference;
			}
		}
	};
} // namespace slc
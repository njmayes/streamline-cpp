#pragma once

#include "Property.h"
#include "Method.h"

#include <algorithm>
#include <ranges>
#include <stdexcept>
#include <vector>

namespace sl {

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
			if ( mInfo->name != TypeTraits< std::remove_cvref_t< T > >::Name )
				throw BadReflectionCastException( TypeTraits< std::remove_cvref_t< T > >::Name, mInfo->name );

			auto ctr = FindConstructor< Args&&... >();
			if ( not ctr )
				throw UnreflectedTargetException< Args... >( mInfo->name );

			std::vector< Instance > instanced_args;
			instanced_args.reserve( sizeof...( Args ) );

			( [ & ]() {
				instanced_args.emplace_back( reflect::MakeInstance( std::forward< Args >( args ) ) );
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
			return IsSubclassOf( Type::Get< T >() );
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
				return Type( T::_reflection_data::Info );
			}
			else
			{
				return Type( reflect::Reflection::GetInfo< T >() );
			}
		}

		static Type Get( std::string_view name )
		{
			return Type( reflect::Reflection::GetInfo( name ) );
		}

	private:
		template < typename... Args >
		const ConstructorInfo* FindConstructor() const
		{
			using ArgTypes = TypeList< Args... >;

			auto is_convertible = [ & ]< std::size_t I >( const TypeRef& ctr_arg ) {
				using ArgType = typename ArgTypes::template Type< I >;
				return IsConvertibleTo< ArgType >( ctr_arg );
			};

			auto match_param = [ & ]< std::size_t... Is >( const ConstructorInfo& ctr, std::index_sequence< Is... > ) {
				return ( ... and is_convertible.template operator()< Is >( ctr.arguments[ Is ] ) );
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
		TypeInfo const* mInfo = nullptr;

		template < typename Arg >
		static bool IsConvertibleTo( const TypeRef& target )
		{
			using Traits = TypeTraits< Arg >;
			using BaseTraits = TypeTraits< std::remove_cvref_t< Arg > >;

			SLC_TODO( "Support conversions between types, not just between value categories of same type" );
			if ( BaseTraits::Name != target.base->name )
				return false;

			auto target_is_ref = target.ref != RefKind::None;

			if constexpr ( Traits::IsLValueReference )
			{
				if constexpr ( Traits::IsConst )
				{
					// const& parameter:
					// - accepts const& argument to const& parameter
					// - accepts lvalue argument to by-value parameter (copy)
					return ( target.is_const and target.ref == RefKind::LValue ) or
						   ( not target_is_ref and target.base->rttt.is_copy_constructible );
				}
				else
				{
					// & parameter:
					// - accepts & argument to & or const& parameter
					// - accepts lvalue argument to by-value parameter (copy)
					return ( target.ref == RefKind::LValue ) or
						   ( not target_is_ref and target.base->rttt.is_copy_constructible );
				}
			}
			else if constexpr ( Traits::IsRValueReference )
			{
				// && parameter:
				// - accepts && argument to && parameter
				// - accepts rvalue argument to by-value parameter (move)
				return ( target.ref == RefKind::RValue ) or
					   ( not target_is_ref and target.base->rttt.is_move_constructible );
			}
			else
			{
				// Value argument:
				// - can bind to by-value / (const&) / (&) parameters via copy
				// - cannot satisfy && parameter
				if ( target.ref == RefKind::RValue )
					return false;

				// If parameter is by-value, any value arg is fine.
				if ( not target_is_ref )
					return true;

				// If parameter is reference:
				// - & param needs lvalue (we have a value here) -> allow only if copyable and target is const&
				//   (since we can create a temporary and bind const&)
				if ( target.ref == RefKind::LValue )
				{
					if ( target.is_const )
						return target.base->rttt.is_copy_constructible;
					return false;
				}

				return false;
			}
		}
	};
} // namespace sl

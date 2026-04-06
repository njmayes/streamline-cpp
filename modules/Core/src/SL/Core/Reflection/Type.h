#pragma once

#include "Property.h"
#include "Method.h"

#include <algorithm>
#include <ranges>
#include <stdexcept>
#include <vector>

namespace sl {

	/// <summary>
	/// <para>
	/// The main interface for runtime reflection. Retrieve the reflection data for a type T using Type::Get&lt;T&gt;() or Type::Get("T").
	/// </para>
	/// <para>
	/// In order to reflect a type, use the SL_REFLECT_CLASS macro in the class scope, listing constructors using the SL_CTR macro, and members using their unqualified name.
	/// default, copy, and move constructors as well as the destructor are reflected by default if they satisfy the respective type traits, e.g. std::is_default_constructible.
	/// </para>
	/// </summary>
	///
	/// <example>
	/// struct Foo
	/// {
	///		Foo(double, int) {}
	///
	///		int bar;
	///		void baz() {}
	///
	///		SL_REFLECT_CLASS(Foo,
	///			SL_CTR(double, int),
	///			bar, baz
	///		)
	/// };
	/// </example>
	class Type
	{
	public:
		Type( const TypeRef& type )
			: mInfo( type )
		{
		}

		Type( const TypeInfo* type )
		{
			mInfo.base = type;
		}

		std::string GetName() const;

		template < CanReflect T, typename... Args >
			requires std::constructible_from< T, Args... >
		T Instantiate( Args&&... args ) const
		{
			if ( mInfo.base->name != TypeTraits< T >::BaseName )
				throw BadReflectionCastException( TypeTraits< std::remove_cvref_t< T > >::Name, mInfo.base->name );

			auto ctr = FindConstructor< Args&&... >();
			if ( not ctr )
				throw UnreflectedTargetException< Args... >( mInfo.base->name );

			std::vector< Instance > instanced_args;
			instanced_args.reserve( sizeof...( Args ) );

			( [ & ]() {
				instanced_args.emplace_back( reflect::MakeInstance( std::forward< Args >( args ) ) );
			}(),
			  ... );

			auto instance = ctr->invoker( std::move( instanced_args ) );
			return instance.data.template Get< T >();
		}

		Property GetProperty( std::string_view name ) const;
		std::vector< Property > GetProperties() const;

		Method GetMethod( std::string_view name ) const;
		std::vector< Method > GetMethods() const;

		RuntimeTypeTraits const& GetTraits() const
		{
			return *mInfo.base->rttt;
		}

		auto operator<=>( const Type& ) const = default;
		operator bool() const
		{
			return mInfo;
		}

		std::string ToString() const;
		std::string ToString( Instance obj ) const;

		template < CanReflect T >
		std::string ToString( T const& obj ) const
		{
			if ( mInfo.base->name != TypeTraits< T >::BaseName )
				throw BadReflectionCastException( TypeTraits< std::remove_cvref_t< T > >::Name, mInfo.base->name );
			return ToString( reflect::MakeInstance( std::forward < decltype( obj ) >( obj ) ) );
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
				return Type( reflect::Reflection::GetTypeRef< T >() );
			}
		}

		static Type Get( std::string_view name )
		{
			return Type( reflect::Reflection::GetTypeRef( name ) );
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

			auto it = std::ranges::find_if( mInfo.base->constructors, match_parameters );
			return it != mInfo.base->constructors.end() ? &( *it ) : nullptr;
		}

		template < typename Arg >
		static bool IsConvertibleTo( TypeRef const& target )
		{
			using Traits = TypeTraits< Arg >;
			using BaseTraits = TypeTraits< std::remove_cvref_t< Arg > >;

			// SL_TODO( "Support conversions between types, not just between value categories of same type" );
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
						   ( not target_is_ref and target.base->rttt->is_copy_constructible );
				}
				else
				{
					// & parameter:
					// - accepts & argument to & or const& parameter
					// - accepts lvalue argument to by-value parameter (copy)
					return ( target.ref == RefKind::LValue ) or
						   ( not target_is_ref and target.base->rttt->is_copy_constructible );
				}
			}
			else if constexpr ( Traits::IsRValueReference )
			{
				// && parameter:
				// - accepts && argument to && parameter
				// - accepts rvalue argument to by-value parameter (move)
				return ( target.ref == RefKind::RValue ) or
					   ( not target_is_ref and target.base->rttt->is_move_constructible );
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
						return target.base->rttt->is_copy_constructible;
					return false;
				}

				return false;
			}
		}

	private:
		TypeRef mInfo{};
	};
} // namespace sl

namespace std {

	template <>
	struct formatter< sl::Type, char >
	{
		constexpr auto parse( format_parse_context& ctx )
		{
			auto it = ctx.begin();
			if ( it != ctx.end() && *it != '}' )
				throw format_error( "Invalid format for sl::Type" );
			return it;
		}

		template < typename FormatContext >
		auto format( sl::Type const& type, FormatContext& ctx ) const
		{
			return format_to( ctx.out(), "{}", type.ToString() );
		};
	};
} // namespace std
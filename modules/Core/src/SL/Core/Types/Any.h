#pragma once

#include <any>
#include <functional>
#include <type_traits>
#include <utility>

namespace sl {

	class Any
	{
	public:
		Any() = default;

		template < typename T >
		Any( T&& val )
		{
			using Traits = TypeTraits< T >;
			if constexpr ( Traits::IsLValueReference )
			{
				mValue = std::ref( val );
			}
			else
			{
				mValue = std::move( val );
			}
		}

		bool HasValue() const
		{
			return mValue.has_value();
		}

		template < typename T >
		T Get() const
		{
			using Traits = TypeTraits< T >;

			using BaseType = std::remove_cvref_t< T >;
			using BaseTypeKeepConst = std::remove_reference_t< T >;
			using RefType = std::add_lvalue_reference_t< BaseTypeKeepConst >;

			if constexpr ( Traits::IsLValueReference )
			{
				if constexpr ( Traits::IsConst )
				{
					using ConstRefWrapperType = std::reference_wrapper< const BaseTypeKeepConst >;
					using NonConstRefWrapperType = std::reference_wrapper< std::remove_const_t< BaseTypeKeepConst > >;

					// const& types may be constructible from value type, so const& std::reference_wrapper cast may not always work.
					if ( auto const_ref = std::any_cast< ConstRefWrapperType >( &mValue ) )
					{
						return const_ref->get();
					}
					else if ( auto ref = std::any_cast< NonConstRefWrapperType >( &mValue ) )
					{
						return ref->get();
					}
					else
					{
						return std::any_cast< RefType >( mValue );
					}
				}
				else
				{
					// Non-const lvalue ref from a const Any is ill-formed to provide.
					// This mirrors "you can't get T& out of const container" semantics.
					static_assert( !Traits::IsLValueReference || Traits::IsConst, "Cannot Get<T&>() from const Any. Use Get<const T&>() instead." );
					throw std::bad_any_cast();
				}
			}
			else if constexpr ( Traits::IsRValueReference )
			{
				// Can't move from const Any meaningfully.
				static_assert( !Traits::IsRValueReference, "Cannot Get<T&&>() from const Any. Use Get<T>() or Get<const T&>() instead." );
				throw std::bad_any_cast();
			}
			else
			{
				// Value types can be constructed from any value category (provided it is copy/move constructible) so require more thorough checks
				using RefWrapperType = std::reference_wrapper< BaseType >;
				using ConstRefWrapperType = std::reference_wrapper< const BaseType >;

				if ( auto value = std::any_cast< BaseType >( &mValue ) )
				{
					// copy (cannot move from const Any)
					return *value;
				}
				else if ( auto const_reference_wrapper = std::any_cast< ConstRefWrapperType >( &mValue ) )
				{
					return const_reference_wrapper->get();
				}
				else if ( auto reference_wrapper = std::any_cast< RefWrapperType >( &mValue ) )
				{
					return reference_wrapper->get();
				}
				else
				{
					throw std::bad_any_cast();
				}
			}
		}

		template < typename T >
		T Get()
		{
			using Traits = TypeTraits< T >;

			using BaseType = std::remove_cvref_t< T >;
			using BaseTypeKeepConst = std::remove_reference_t< T >;
			using RefType = std::add_lvalue_reference_t< BaseTypeKeepConst >;

			if constexpr ( Traits::IsLValueReference )
			{
				using RefWrapperType = std::reference_wrapper< BaseTypeKeepConst >;
				if constexpr ( Traits::IsConst )
				{
					using NonConstRefWrapperType = std::reference_wrapper< std::remove_const_t< BaseTypeKeepConst > >;

					// const& types may be constructible from value type, so const& std::reference_wrapper cast may not always work.
					if ( auto const_ref = std::any_cast< RefWrapperType >( &mValue ) )
					{
						return const_ref->get();
					}
					else if ( auto ref = std::any_cast< NonConstRefWrapperType >( &mValue ) )
					{
						return ref->get();
					}
					else
					{
						return std::any_cast< RefType >( mValue );
					}
				}
				else
				{
					return std::any_cast< RefWrapperType >( mValue ).get();
				}
			}
			else if constexpr ( Traits::IsRValueReference )
			{
				return std::move( std::any_cast< RefType >( mValue ) );
			}
			else
			{
				// Value types can be constructed from any value category (provided it is copy/move constructible) so require more thorough checks
				using RefWrapperType = std::reference_wrapper< BaseType >;
				using ConstRefWrapperType = std::reference_wrapper< const BaseType >;

				if ( auto value = std::any_cast< BaseType >( &mValue ) )
				{
					// Try by value/move first
					return std::move( *value );
				}
				else if ( auto const_reference_wrapper = std::any_cast< ConstRefWrapperType >( &mValue ) )
				{
					// Then try by const&
					return const_reference_wrapper->get();
				}
				else if ( auto reference_wrapper = std::any_cast< RefWrapperType >( &mValue ) )
				{
					// Then try by &
					return reference_wrapper->get();
				}
				else
				{
					// Could not convert, throw exception
					throw std::bad_any_cast();
				}
			}
		}

	private:
		std::any mValue;
	};
} // namespace sl

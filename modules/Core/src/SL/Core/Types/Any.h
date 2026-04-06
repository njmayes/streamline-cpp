#pragma once

#include "Detail/Any.h"

#include <any>
#include <functional>
#include <type_traits>
#include <utility>

namespace sl {

	class Any
	{
	public:
		Any() = default;

		Any( Any const& ) = default;
		Any( Any&& ) noexcept = default;

		Any& operator=( Any const& ) = default;
		Any& operator=( Any&& ) noexcept = default;

		template < typename T >
		Any( T&& value )
		{
			Set( std::forward< T >( value ) );
		}

		template < typename T >
		Any& operator=( T&& value )
		{
			Set( std::forward< T >( value ) );
			return *this;
		}

		bool HasValue() const noexcept
		{
			return mValue.has_value();
		}

		template < typename T >
		void Set( T&& value )
		{
			using Traits = TypeTraits< T >;
			using BaseType = typename Traits::BaseType;

			if constexpr ( Traits::IsLValueReference )
			{
				auto ref = std::ref( value );
				mValue = ref;
				ConfigurePointerGetterForStoredType< decltype( ref ) >();
			}
			else if constexpr ( detail::IsReferenceWrapper< BaseType >::Value )
			{
				mValue = std::forward< T >( value );
				ConfigurePointerGetterForStoredType< BaseType >();
			}
			else
			{
				mValue = BaseType( std::forward< T >( value ) );
				ConfigurePointerGetterForStoredType< BaseType >();
			}
		}

		template < typename T >
		bool Is() const noexcept
		{
			using Traits = TypeTraits< T >;
			using BaseType = typename Traits::BaseType;
			using MutableRef = std::reference_wrapper< BaseType >;
			using ConstRef = std::reference_wrapper< BaseType const >;
			using NoCvRef = std::remove_cvref_t< T >;

			if constexpr ( std::same_as< NoCvRef, void* > || std::same_as< NoCvRef, void const* > )
			{
				return static_cast< bool >( mPointerGetter );
			}
			else if constexpr ( Traits::IsLValueReference )
			{
				if constexpr ( Traits::IsConst )
					return mValue.type() == typeid( MutableRef ) || mValue.type() == typeid( ConstRef );
				else
					return mValue.type() == typeid( MutableRef );
			}
			else if constexpr ( Traits::IsRValueReference )
			{
				if constexpr ( Traits::IsConst )
					return mValue.type() == typeid( MutableRef ) || mValue.type() == typeid( ConstRef );
				else
					return mValue.type() == typeid( MutableRef );
			}
			else
			{
				return mValue.type() == typeid( BaseType ) ||
					   mValue.type() == typeid( MutableRef ) ||
					   mValue.type() == typeid( ConstRef );
			}
		}

		template < typename T >
		decltype( auto ) Get()
		{
			using Traits = TypeTraits< T >;
			using BaseType = typename Traits::BaseType;
			using NoCvRef = std::remove_cvref_t< T >;

			if constexpr ( std::same_as< NoCvRef, void* > || std::same_as< NoCvRef, void const* > )
			{
				if ( !mPointerGetter )
					throw std::bad_any_cast();

				void const* raw = mPointerGetter( mValue );

				if constexpr ( std::same_as< NoCvRef, void* > )
					return const_cast< void* >( raw );
				else
					return raw;
			}
			else if constexpr ( std::is_pointer_v< BaseType > && !std::is_function_v< std::remove_pointer_t< BaseType > > )
			{
				if ( !mPointerGetter )
					throw std::bad_any_cast();

				void const* raw = mPointerGetter( mValue );

				if constexpr ( Traits::IsLValueReference )
				{
					// Need typed path for reference to stored pointer object.
					using MutableRef = std::reference_wrapper< BaseType >;
					using ConstRef = std::reference_wrapper< BaseType const >;

					if constexpr ( Traits::IsConst )
					{
						if ( auto* ref = std::any_cast< ConstRef >( &mValue ) )
							return static_cast< BaseType const& >( ref->get() );

						if ( auto* ref = std::any_cast< MutableRef >( &mValue ) )
							return static_cast< BaseType const& >( ref->get() );

						if ( auto* value = std::any_cast< BaseType >( &mValue ) )
							return static_cast< BaseType const& >( *value );

						throw std::bad_any_cast();
					}
					else
					{
						if ( auto* ref = std::any_cast< MutableRef >( &mValue ) )
							return static_cast< BaseType& >( ref->get() );

						if ( auto* value = std::any_cast< BaseType >( &mValue ) )
							return static_cast< BaseType& >( *value );

						throw std::bad_any_cast();
					}
				}
				else if constexpr ( Traits::IsRValueReference )
				{
					if constexpr ( Traits::IsConst )
						return static_cast< BaseType const&& >( reinterpret_cast< BaseType >( const_cast< void* >( raw ) ) );
					else
						return static_cast< BaseType&& >( reinterpret_cast< BaseType >( const_cast< void* >( raw ) ) );
				}
				else
				{
					return reinterpret_cast< BaseType >( const_cast< void* >( raw ) );
				}
			}
			else if constexpr ( Traits::IsLValueReference )
			{
				if ( !mPointerGetter )
					throw std::bad_any_cast();

				auto raw = mPointerGetter( mValue );

				if constexpr ( Traits::IsConst )
				{
					auto typed = reinterpret_cast< BaseType const* >( raw );
					return static_cast< BaseType const& >( *typed );
				}
				else
				{
					auto typed = reinterpret_cast< BaseType* >( const_cast< void* >( raw ) );
					return static_cast< BaseType& >( *typed );
				}
			}
			else if constexpr ( Traits::IsRValueReference )
			{
				if ( !mPointerGetter )
					throw std::bad_any_cast();

				auto raw = mPointerGetter( mValue );

				if constexpr ( Traits::IsConst )
				{
					auto typed = reinterpret_cast< BaseType const* >( raw );
					return static_cast< BaseType const&& >( std::move( *typed ) );
				}
				else
				{
					auto typed = reinterpret_cast< BaseType* >( const_cast< void* >( raw ) );
					return static_cast< BaseType&& >( std::move( *typed ) );
				}
			}
			else
			{
				if ( !mPointerGetter )
					throw std::bad_any_cast();

				auto raw = mPointerGetter( mValue );

				if constexpr ( std::is_const_v< BaseType > )
				{
					auto typed = reinterpret_cast< BaseType const* >( raw );
					return BaseType( *typed );
				}
				else
				{
					auto typed = reinterpret_cast< BaseType* >( const_cast< void* >( raw ) );
					return BaseType( *typed );
				}
			}
		}

		template < typename T >
		decltype( auto ) Get() const
		{
			using Traits = TypeTraits< T >;
			using BaseType = typename Traits::BaseType;
			using NoCvRef = std::remove_cvref_t< T >;

			static_assert( !Traits::IsRValueReference, "Cannot call Get<T&&>() on const Any" );

			if constexpr ( std::same_as< NoCvRef, void* > || std::same_as< NoCvRef, void const* > )
			{
				if ( !mPointerGetter )
					throw std::bad_any_cast();

				void const* raw = mPointerGetter( mValue );

				if constexpr ( std::same_as< NoCvRef, void* > )
					return const_cast< void* >( raw );
				else
					return raw;
			}
			else if constexpr ( std::is_pointer_v< BaseType > && !std::is_function_v< std::remove_pointer_t< BaseType > > )
			{
				if ( !mPointerGetter )
					throw std::bad_any_cast();

				void const* raw = mPointerGetter( mValue );

				if constexpr ( Traits::IsLValueReference )
				{
					// Still need typed path here if you want a reference to the stored pointer object,
					// not just the pointer value.
					using MutableRef = std::reference_wrapper< BaseType >;
					using ConstRef = std::reference_wrapper< BaseType const >;

					if ( auto const* ref = std::any_cast< ConstRef >( &mValue ) )
						return static_cast< BaseType const& >( ref->get() );

					if ( auto const* ref = std::any_cast< MutableRef >( &mValue ) )
						return static_cast< BaseType const& >( ref->get() );

					if ( auto const* value = std::any_cast< BaseType >( &mValue ) )
						return static_cast< BaseType const& >( *value );

					throw std::bad_any_cast();
				}
				else
				{
					return reinterpret_cast< BaseType >( raw );
				}
			}
			else if constexpr ( Traits::IsLValueReference )
			{
				if ( !mPointerGetter )
					throw std::bad_any_cast();

				auto raw = mPointerGetter( mValue );
				auto typed = reinterpret_cast< BaseType const* >( raw );
				return static_cast< BaseType const& >( *typed );
			}
			else
			{
				if ( !mPointerGetter )
					throw std::bad_any_cast();

				auto raw = mPointerGetter( mValue );
				auto typed = reinterpret_cast< BaseType const* >( raw );
				return BaseType( *typed );
			}
		}

	private:
		template < typename TValue >
		static void const* EraseToVoidPointer( TValue& value ) noexcept
		{
			using Value = std::remove_reference_t< TValue >;

			if constexpr ( std::is_pointer_v< Value > && !std::is_function_v< std::remove_pointer_t< Value > > )
			{
				return static_cast< void const* >( value );
			}
			else
			{
				return static_cast< void const* >( std::addressof( value ) );
			}
		}

		template < typename TStored >
		void ConfigurePointerGetterForStoredType()
		{
			if constexpr ( detail::IsReferenceWrapper< TStored >::Value )
			{
				mPointerGetter = []( std::any const& value ) -> void const* {
					auto const* ref = std::any_cast< TStored >( &value );
					if ( ref == nullptr )
						throw std::bad_any_cast();

					return EraseToVoidPointer( ref->get() );
				};
			}
			else
			{
				mPointerGetter = []( std::any const& value ) -> void const* {
					auto const* obj = std::any_cast< TStored >( &value );
					if ( obj == nullptr )
						throw std::bad_any_cast();

					return EraseToVoidPointer( *obj );
				};
			}
		}

	private:
		std::any mValue;
		std::function< void const*( std::any const& ) > mPointerGetter;
	};
} // namespace sl
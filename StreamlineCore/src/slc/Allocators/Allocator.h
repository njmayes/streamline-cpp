#pragma once

#include "slc/Common/Base.h"

namespace slc {

	/// <summary>
	/// Base allocator interface.
	/// Inherited classes must provide overrides to retrieve and free fixed size blocks of memory, as well as a max size override.
	/// </summary>
	class IAllocator : public RefCounted
	{
	public:
		SCONSTEXPR std::size_t SCALE_FACTOR = 2;

		virtual ~IAllocator() = default;

		virtual bool CanAllocate( std::size_t size ) const = 0;
		virtual void ForceReallocate() = 0;

		virtual void* Alloc( size_t size ) = 0;
		virtual void Free( void* ptr = nullptr ) = 0;
	};

	template < typename T >
	class Allocator
	{
	public:
		using value_type = T;
		using pointer = T*;
		using const_pointer = const T*;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;

		using propagate_on_container_copy_assignment = std::false_type;
		using propagate_on_container_move_assignment = std::true_type;
		using propagate_on_container_swap = std::true_type;
		using is_always_equal = std::false_type;

		Allocator() noexcept = default;

		template < DerivedFromOnly< IAllocator > Impl >
		Allocator( Ref< Impl > allocator ) noexcept
			: mImpl( std::move( allocator ) )
		{}

		template < class U >
		Allocator( const Allocator< U >& other ) noexcept
			: mImpl( other.mAllocator )
		{}

		T* allocate( std::size_t n )
		{
			if ( n > ( SIZE_MAX / sizeof( T ) ) )
				throw std::bad_array_new_length();
			void* p = mImpl ? mImpl->Alloc( n * sizeof( T ) ) : nullptr;
			if ( !p )
				throw std::bad_alloc();
			return static_cast< pointer >( p );
		}

		void deallocate( T* ptr, std::size_t n )
		{
			if ( mImpl )
				mImpl->Free( ptr );
		}

		template < class T1, class T2 >
		friend bool operator==( const Allocator< T1 >& a, const Allocator< T2 >& b ) noexcept;

		template < class T1, class T2 >
		friend bool operator!=( const Allocator< T1 >& a, const Allocator< T2 >& b ) noexcept;

	private:
		Ref< IAllocator > mImpl{};

		template < class >
		friend class Allocator;
	};

	template < class T1, class T2 >
	bool operator==( const Allocator< T1 >& a, const Allocator< T2 >& b ) noexcept
	{
		return a.mImpl.get() == b.mImpl.get();
	}
	template < class T1, class T2, class I >
	bool operator!=( const Allocator< T1 >& a, const Allocator< T2 >& b ) noexcept
	{
		return !( a == b );
	}
} // namespace slc
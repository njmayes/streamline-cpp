#pragma once

#include "slc/Common/Base.h"

namespace slc {

	/// <summary>
	/// Base allocator interface.
	/// Inherited classes must provide overrides to retrieve and free fixed size blocks of memory, as well as a max size override.
	/// </summary>
	class IAllocator
	{
	public:
		SCONSTEXPR std::size_t SCALE_FACTOR = 2;

		virtual ~IAllocator() = default;

		template < typename T, typename... Args >
		T* Alloc( Args&&... args )
		{
			T* ptr = static_cast< T* >( AllocImpl( sizeof( T ) ) );
			new ( ptr ) T( std::forward< Args >( args )... );
			return ptr;
		}

		template < typename T >
		void Free( T* ptr )
		{
			ptr->~T();
			FreeImpl( static_cast< void* >( ptr ) );
		}

		virtual void Reset()
		{}

		virtual void ForceReallocate()
		{}
		virtual std::size_t MaxSize() const = 0;

	protected:
		virtual void* AllocImpl( size_t size ) = 0;
		virtual void FreeImpl( void* ptr = nullptr ) = 0;
	};
} // namespace slc
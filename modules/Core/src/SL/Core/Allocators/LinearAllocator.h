#pragma once

#include "Allocator.h"

#include "SL/Core/Common/Base.h"

namespace sl {

	template< typename T >
	concept LinearAllocatable = std::is_trivially_copyable_v< T > and std::is_trivially_destructible_v< T >;

	/// <summary>
	/// A simple arena allocator for objects of type T.
	/// </summary>
	/// <typeparam name="T"></typeparam>
	template < typename T >
		requires LinearAllocatable< T >
	class LinearAllocator : public IAllocator
	{
	public:
		LinearAllocator( size_t size )
			: mMaxSize( size ), mMemBlock( static_cast< T* >( ::operator new( mMaxSize * sizeof( T ) ) ) ), mHead( mMemBlock )
		{
		}

		~LinearAllocator() override
		{
			::operator delete( mMemBlock );
		}

		LinearAllocator( const LinearAllocator& ) = delete;
		LinearAllocator( LinearAllocator&& other ) noexcept
			: mMaxSize( other.mMaxSize ), mMemBlock( std::exchange( other.mMemBlock, nullptr ) ), mHead( mMemBlock )
		{}

		auto operator=( const LinearAllocator& ) = delete;
		auto operator=( LinearAllocator&& other ) noexcept
		{
			if ( mMemBlock != other.mMemBlock )
			{
				::operator delete( mMemBlock );
			}

			mMaxSize = other.mMaxSize;
			mMemBlock = std::exchange( other.mMemBlock, nullptr );
			mHead = other.mHead;

			return *this;
		}

		bool CanAllocate( std::size_t size ) const override
		{
			return ( mHead + size ) <= ( mMemBlock + mMaxSize );
		}

		void ForceReallocate() override
		{
			Reallocate();
		}

		void* Alloc( size_t size ) override
		{
			if ( size % sizeof( T ) != 0 )
				return nullptr;

			auto count = size / sizeof( T );
			if ( not CanAllocate( count ) )
				Reallocate();

			auto result = mHead;
			mHead += count;
			return result;
		}

		void Free( void* = nullptr ) override
		{
			mHead = mMemBlock;
		}

	private:
		void Reallocate()
		{
			T* tmp = mMemBlock;
			ptrdiff_t offset = mHead - mMemBlock;
			size_t tmp_size = mMaxSize;

			mMaxSize *= SCALE_FACTOR;
			mMemBlock = static_cast< T* >( ::operator new( mMaxSize * sizeof( T ) ) );
			mHead = mMemBlock + offset;

			std::memcpy( mMemBlock, tmp, tmp_size * sizeof( T ) );
			::operator delete( tmp );
		}

	private:
		std::size_t mMaxSize = 0;

		T* mMemBlock = nullptr;
		T* mHead = nullptr;
	};
} // namespace sl
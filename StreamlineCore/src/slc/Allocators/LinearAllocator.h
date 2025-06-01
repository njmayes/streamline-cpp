#pragma once

#include "Allocator.h"

#include "slc/Common/Base.h"

namespace slc {

	/// <summary>
	/// A simple arena allocator for objects of type T.
	/// </summary>
	/// <typeparam name="T"></typeparam>
	template < typename T >
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
		}

		std::size_t MaxSize() const override
		{
			return mMaxSize;
		}
		void ForceReallocate() override
		{
			Reallocate();
		}

		void Reset()
		{
			mHead = mMemBlock;
		}

	protected:
		void* AllocImpl( size_t size ) override
		{
			if ( size != sizeof( T ) )
				return nullptr;

			if ( mHead == ( mMemBlock + mMaxSize ) )
				Reallocate();

			return mHead++;
		}

		void FreeImpl( void* = nullptr ) override
		{
			mHead = mMemBlock;
		}

	private:
		void Reallocate()
		{
			T* tmp = mMemBlock;
			ptrdiff_t offset = mHead - mMemBlock;
			size_t tmpSize = mMaxSize;

			mMaxSize *= SCALE_FACTOR;
			mMemBlock = static_cast< T* >( ::operator new( mMaxSize * sizeof( T ) ) );
			mHead = mMemBlock + offset;

			memcpy( mMemBlock, tmp, tmpSize );
			::operator delete( tmp );
		}

	private:
		std::size_t mMaxSize = 0;

		T* mMemBlock = nullptr;
		T* mHead = nullptr;
	};
} // namespace slc
#pragma once

#include "Allocator.h"

namespace slc {

	template < typename T >
	class StackAllocator : public IAllocator
	{
	public:
		struct AllocHeader
		{
			std::size_t size;
		};

		static constexpr size_t HeaderSize = sizeof( AllocHeader );

		StackAllocator( std::size_t size )
			: mMaxSize( size ), mMemBlock( ::operator new( mMaxSize ) ), mHead( mMemBlock )
		{}

		~StackAllocator() override
		{
			::operator delete( mMemBlock );
		}

		StackAllocator( const StackAllocator& ) = delete;
		StackAllocator( StackAllocator&& other ) noexcept
			: mMaxSize( other.mMaxSize ), mMemBlock( std::exchange( other.mMemBlock, nullptr ) ), mHead( mMemBlock )
		{}

		auto operator=( const StackAllocator& ) = delete;
		auto operator=( StackAllocator&& other ) noexcept
		{
			if ( mMemBlock != other.mMemBlock )
			{
				::operator delete( mMemBlock );
			}

			mMaxSize = other.mMaxSize;
			mMemBlock = std::exchange( other.mMemBlock, nullptr );
			mHead = other.mHead;
		}

		bool CanAllocate() const override
		{
			return ( mHead + HeaderSize + sizeof( T ) ) < mMemBlock + mMaxSize;
		}

		void ForceReallocate() override
		{
			Reallocate();
		}

		void* Alloc( size_t size ) override
		{
			if ( CanAllocate() )
				Reallocate();

			std::memcpy( mHead, &size, HeaderSize );

			mHead += HeaderSize;
			void* memblock = mHead;

			mHead += size;
			return memblock;
		}

		void Free( void* ptr ) override
		{
			Byte* bytes = reinterpret_cast< Byte* >( ptr );

			AllocHeader header{};
			std::memcpy( &header, bytes - HeaderSize, HeaderSize );

			mHead -= ( header.size + HeaderSize );
		}

	private:
		void Reallocate()
		{
			Byte* tmp = mMemBlock;
			ptrdiff_t offset = mHead - mMemBlock;
			size_t tmp_size = mMaxSize;

			mMaxSize *= SCALE_FACTOR;
			mMemBlock = static_cast< Byte* >( ::operator new( mMaxSize ) );
			mHead = mMemBlock + offset;

			memcpy( mMemBlock, tmp, tmp_size );
			::operator delete( tmp );
		}

	private:
		std::size_t mMaxSize = 0;

		T* mMemBlock = nullptr;
		T* mHead = nullptr;
	};
} // namespace slc
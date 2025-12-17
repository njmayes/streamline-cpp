#pragma once

#include "Allocator.h"

#include "SL/Core/Common/Base.h"

namespace sl {

	template < typename T >
	class BlockAllocator : public IAllocator
	{
	public:
		struct alignas( T ) BlockData
		{
			Byte bytes[ sizeof( T ) ];
		};

		struct Block
		{
			std::size_t size;
			BlockData data;
		};

		using BlockMap = std::map< std::size_t, std::vector< Block* > >;

		SCONSTEXPR auto BLOCK_SIZE = sizeof( Block );

		BlockAllocator( size_t count )
			: mMaxSize( count )
		{
			void* memory = ::operator new( mMaxSize * BLOCK_SIZE, std::align_val_t( alignof( Block ) ) );
			mMemBlock = static_cast< Block* >( memory );
			mMemBlock->size = mMaxSize;

			for ( std::size_t i = 1; i <= mMaxSize; i++ )
			{
				auto max_count = mMaxSize / i;
				mBlockSizes[ i ].reserve( max_count );
			}

			mBlockSizes[ mMaxSize ].push_back( mMemBlock );
		}

		~BlockAllocator() override
		{
			::operator delete( mMemBlock, std::align_val_t( alignof( Block ) ) );
		}

		BlockAllocator( const BlockAllocator& ) = delete;
		BlockAllocator( BlockAllocator&& other ) noexcept
			: mMaxSize( other.mMaxSize )
			, mMemBlock( std::exchange( other.mMemBlock, nullptr ) )
			, mInUse( std::exchange( other.mInUse, {}  ) )
			, mBlockSizes( std::exchange( other.mBlockSizes, {}  ) )
		{}

		auto operator=( const BlockAllocator& ) = delete;
		auto operator=( BlockAllocator&& other ) noexcept
		{
			mMaxSize = other.mMaxSize;
			mMemBlock = std::exchange( other.mMemBlock, nullptr );
			mInUse = std::exchange( other.mInUse, {} );
			mBlockSizes = std::exchange( other.mBlockSizes, {} );

			return *this;
		}

		bool CanAllocate( std::size_t size ) const override
		{
			auto it = FindAvailableBlocks( size );
			return HasValidBlock( it );
		}

		void ForceReallocate() override
		{
			Reallocate();
		}

		void* Alloc( size_t size ) override
		{
			if ( size == 0 )
				return nullptr;

			auto blocks = TryGetBlocks( size );
			if ( not blocks )
				return nullptr;

			auto block_size = ( *blocks )->first;
			auto& block_list = ( *blocks )->second;

			auto free_block = block_list.back();
			block_list.pop_back();

			auto extra = block_size - size;
			if ( extra > 0 )
			{
				auto extra_block = free_block + size;
				extra_block->size = extra;
				mBlockSizes[ extra ].push_back( extra_block );
			}

			mInUse.insert( free_block );

			free_block->size = size;
			return reinterpret_cast< T* >( free_block->data.bytes );
		}

		void Free( void* ptr ) override
		{
			Byte* block_data = reinterpret_cast< Byte* >( ptr );
			Block* block = reinterpret_cast< Block* >( block_data - offsetof( Block, data.bytes ) );

			mInUse.erase( block );
			mBlockSizes[ block->size ].push_back( block );
		}

	private:
		void Coalesce()
		{
			mBlockSizes.clear();

			auto ptr = mMemBlock;
			auto end = mMemBlock + mMaxSize;

			while ( ptr < end )
			{
				auto next = ptr + ptr->size;

				if ( not mInUse.contains( ptr ) )
				{
					while ( next < end and not mInUse.contains( next ) )
						next += next->size;

					auto merged_size = next - ptr;

					ptr->size = merged_size;
					mBlockSizes[ merged_size ].push_back( ptr );
				}

				ptr = next;
			}
		}

		void Reallocate()
		{
			Block* old_base = mMemBlock;
			size_t old_cap = mMaxSize;

			mMaxSize *= SCALE_FACTOR;

			Block* new_base = static_cast< Block* >( ::operator new( mMaxSize * BLOCK_SIZE, std::align_val_t( alignof( Block ) ) ) );

			mBlockSizes.clear();

			auto in_use = std::move( mInUse );
			mInUse.clear();

			Block* ptr = old_base;
			while ( ptr < ( old_base + old_cap ) )
			{
				auto new_block = new_base + ( ptr - old_base );
				new_block->size = ptr->size;

				if ( in_use.contains( ptr ) )
				{
					if constexpr ( std::is_trivially_copyable_v< T > )
					{
						std::memcpy( new_block, ptr, ptr->size * BLOCK_SIZE );
					}
					else
					{
						SASSERT( std::is_nothrow_move_constructible_v< T >, "Type must be no throw move constructible" );
						auto typed_old_block = reinterpret_cast< T* >( ptr->data.bytes );
						auto typed_new_block = reinterpret_cast< T* >( new_block->data.bytes );

						std::uninitialized_move( typed_old_block, typed_old_block + ptr->size, typed_new_block );
						std::destroy_n( typed_old_block, ptr->size );
					}

					mInUse.insert( new_block );
				}
				else
				{
					mBlockSizes[ new_block->size ].push_back( new_block );
				}

				ptr += ptr->size;
			}

			::operator delete( mMemBlock, std::align_val_t( alignof( Block ) ) );

			mMemBlock = new_base;
		}

		auto FindAvailableBlocks( std::size_t size )
		{
			return std::ranges::lower_bound( mBlockSizes, size, {}, &std::pair< const std::size_t, std::vector< Block* > >::first );
		}
		auto FindAvailableBlocks( std::size_t size ) const
		{
			return std::ranges::lower_bound( mBlockSizes, size, {}, &std::pair< const std::size_t, std::vector< Block* > >::first );
		}

		bool HasValidBlock( auto it ) const
		{
			return it != mBlockSizes.end() and not it->second.empty();
		}

		auto TryGetBlocks( std::size_t size )
		{
			{
				auto blocks = FindAvailableBlocks( size );
				if ( HasValidBlock( blocks ) )
					return blocks;
			}

			Coalesce();

			{
				auto blocks = FindAvailableBlocks( size );
				if ( HasValidBlock( blocks ) )
					return blocks;
			}

			Reallocate();

			{
				auto blocks = FindAvailableBlocks( size );
				if ( HasValidBlock( blocks ) )
					return blocks;
			}

			return {};
		}

	private:
		std::size_t mMaxSize = 0;

		Block* mMemBlock = nullptr;
		std::set< Block* > mInUse;
		std::map< std::size_t, std::vector< Block* > > mBlockSizes;
	};
} // namespace sl
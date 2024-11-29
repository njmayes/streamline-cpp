#pragma once

#include "Allocator.h"

#include "slc/Common/Base.h"

namespace slc {

    template<typename T>
    class PoolAllocator : public IAllocator
    {
    public:
        union Block
        {
            Block* next;
            T value;
        };

        SASSERT(sizeof(T) >= sizeof(Block*), "Free list block size must be smaller than object size.");
        SCONSTEXPR auto BLOCK_SIZE = sizeof(Block);

        PoolAllocator(size_t count)
            : mMaxSize(count)
        {
            void* memory = ::operator new(mMaxSize * BLOCK_SIZE);
            mMemBlock = static_cast<Block*>(memory);
            mHead = static_cast<Block*>(memory);

            for (auto i = 1; i < mMaxSize; i++)
            {
                Block* prev_block = &mMemBlock[i - 1];
                Block* curr_block = &mMemBlock[i];

                prev_block->next = curr_block;
            }
        }

        ~PoolAllocator() override
        {
            ::operator delete(mMemBlock);
        }

        PoolAllocator(const PoolAllocator&) = delete;
        PoolAllocator(PoolAllocator&& other) noexcept
            : mMaxSize(other.mMaxSize), mMemBlock(std::exchange(other.mMemBlock, nullptr)), mHead(mMemBlock) {}

        auto operator=(const PoolAllocator&) = delete;
        auto operator=(PoolAllocator&& other) noexcept
        {
            if (mMemBlock != other.mMemBlock)
            {
                ::operator delete(mMemBlock);
            }

            mMaxSize = other.mMaxSize;
            mMemBlock = std::exchange(other.mMemBlock, nullptr);
            mHead = other.mHead;
        }

        std::size_t MaxSize() const override { return mMaxSize; }
        void ForceReallocate() override { Reallocate(); }

    protected:
        void* AllocImpl(size_t size) override
        {
            if (sizeof(T) != size)
                return nullptr;

            Block* new_value = mHead;
            mHead = mHead->next;
            return &new_value->value;
        }

        void FreeImpl(void* ptr) override
        {
            Block* block = static_cast<Block*>(ptr);
            block->next = mHead;
            mHead = block;
        }

    private:
        void Reallocate()
        {
            Block* tmp = mMemBlock;

            ASSERT(mHead > mMemBlock);
            size_t offset = mHead - mMemBlock;
            size_t tmpSize = mMaxSize;

            mMaxSize *= SCALE_FACTOR;

            void* memory = ::operator new(mMaxSize * BLOCK_SIZE);
            mMemBlock = static_cast<Block*>(memory);
            mHead = mMemBlock + offset;

            std::memcpy(mMemBlock, tmp, tmpSize);
            ::operator delete(tmp);


            for (auto i = offset + 1; i < mMaxSize; i++)
            {
                Block* prev_block = &mMemBlock[i - 1];
                Block* curr_block = &mMemBlock[i];

                prev_block->next = curr_block;
            }
        }

    private:
        std::size_t mMaxSize = 0;

        Block* mMemBlock = nullptr;
        Block* mHead = nullptr;
    };
}
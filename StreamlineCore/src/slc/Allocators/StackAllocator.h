#pragma once

#include "Allocator.h"

namespace slc {

	class StackAllocator : public IAllocator
	{
	public:
		struct AllocHeader
		{
			std::size_t size;
		};

        StackAllocator(std::size_t size)
            : mMaxSize(size), mMemBlock(static_cast<Byte*>(::operator new(mMaxSize))), mHead(mMemBlock)
        {}

        ~StackAllocator() override
        {
            ::operator delete(mMemBlock);
        }

        StackAllocator(const StackAllocator&) = delete;
        StackAllocator(StackAllocator&& other) noexcept
            : mMaxSize(other.mMaxSize), mMemBlock(std::exchange(other.mMemBlock, nullptr)), mHead(mMemBlock) {}

        auto operator=(const StackAllocator&) = delete;
        auto operator=(StackAllocator&& other) noexcept
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
            constexpr size_t HeaderSize = sizeof(AllocHeader);

            if ((mHead + size + HeaderSize) >= mMemBlock + mMaxSize)
                Reallocate();

            std::memcpy(mHead, &size, HeaderSize);

            mHead += HeaderSize;
            void* memblock = mHead;

            mHead += size;
            return memblock;
        }

        void FreeImpl(void* ptr) override
        {
            constexpr size_t HeaderSize = sizeof(AllocHeader);
            Byte* bytes = reinterpret_cast<Byte*>(ptr);

            AllocHeader header{};
            std::memcpy(&header, bytes - HeaderSize, HeaderSize);

            mHead -= (header.size + HeaderSize);
        }

    private:
        void Reallocate()
        {
            Byte* tmp = mMemBlock;
            ptrdiff_t offset = mHead - mMemBlock;
            size_t tmpSize = mMaxSize;

            mMaxSize *= SCALE_FACTOR;
            mMemBlock = static_cast<Byte*>(::operator new(mMaxSize));
            mHead = mMemBlock + offset;

            memcpy(mMemBlock, tmp, tmpSize);
            ::operator delete(tmp);
        }

	private:
        std::size_t mMaxSize = 0;

		Byte* mMemBlock = nullptr;
		Byte* mHead = nullptr;
	};
}
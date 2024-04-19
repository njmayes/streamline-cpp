#pragma once

#include "Allocator.h"

#include "Common/Base.h"

namespace slc {

    /// <summary>
    /// A simple arena allocator for objects of type T.
    /// </summary>
    /// <typeparam name="T"></typeparam>
    template<typename T>
    class PoolAllocator : public IAllocator
    {
    public:
        PoolAllocator(size_t size)
            : mMaxSize(size), mMemBlock(static_cast<T*>(::operator new(mMaxSize * sizeof(T)))), mHead(mMemBlock)
        {
        }

        ~PoolAllocator() override
        {
            ::operator delete(mMemBlock);
        }

        PoolAllocator(const PoolAllocator&) = delete;
        PoolAllocator(PoolAllocator&& other)
            : mMaxSize(other.mMaxSize), mMemBlock(std::exchange(other.mMemBlock, nullptr)), mHead(mMemBlock) {}

        auto operator=(const PoolAllocator&) = delete;
        auto operator=(PoolAllocator&& other)
        {
            if (mMemBlock != other.mMemBlock)
            {
                ::operator delete(mMemBlock);
            }

            mMaxSize = other.mMaxSize;
            mMemBlock = std::exchange(other.mMemBlock, nullptr);
            mHead = other.mHead;
        }

        size_t Size() const override { return mMaxSize; }

        void Free(void* = nullptr) override
        {
            while (mHead != mMemBlock)
            {
                (--mHead)->~T();
            }
        }

        void ForceReallocate() override { Reallocate(); }

    protected:
        void* Alloc(size_t size) override
        {
            if (size != sizeof(T))
                return nullptr;

            if (mHead == (mMemBlock + mMaxSize))
                Reallocate();

            return mHead++;
        }

    private:
        void Reallocate()
        {
            T* tmp = mMemBlock;
            ptrdiff_t offset = mHead - mMemBlock;
            size_t tmpSize = mMaxSize;

            mMaxSize *= 2;
            mMemBlock = static_cast<T*>(::operator new(mMaxSize * sizeof(T)));
            mHead = mMemBlock + offset;
            
            memcpy(mMemBlock, tmp, tmpSize);
            ::operator delete(tmp);
        }

    private:
        size_t mMaxSize = 0;

        T* mMemBlock = nullptr;
        T* mHead = nullptr;
    };
}
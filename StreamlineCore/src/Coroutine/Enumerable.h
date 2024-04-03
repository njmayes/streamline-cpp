#pragma once

#include "Internal/EnumerableInternal.h"

namespace slc {

    template<typename T, typename R>
    concept Enumerable = std::ranges::range<T> or Internal::UserDefinedEnumerable<T, R>;

	template<typename T>
	class Enumerator;


    template<typename T>
    class IEnumerable
    {
    private:
        using BaseType = IEnumerable<T>;

        // To be implemented in derived class if derived type T does not satisfy std::ranges::range<T>.
        // Should be unreachable (see GetEnumeratorInternal)
    public:
        virtual Enumerator<T> GetEnumerator()
        {
            std::unreachable();
            co_return;
        }

        // C# LINQ-like functions
    public:
		template<typename Self, IsPredicate<T&> Func>
        bool All(this Self&& self, Func&& predicate)
        {
            for (T& val : self.GetEnumeratorInternal())
            {
                if (!predicate(val))
                    return false;
            }

            return true;
        }

        template<typename Self, IsPredicate<T&> Func>
        bool Any(this Self&& self, Func&& predicate)
        {
            for (T& val : self.GetEnumeratorInternal())
            {
                if (predicate(val))
                    return true;
            }

            return false;
        }

		template<typename Self>
		Enumerator<T> Append(this Self&& self, const T& newVal)
		{
			for (T& val : self.GetEnumeratorInternal())
			{
				co_yield val;
			}
			co_yield newVal;
		}

		template<typename R, typename Self, typename Func> requires IsFunc<Func, R, T&>
		Enumerator<R> Select(this Self&& self, Func&& op)
		{
			for (T& val : self.GetEnumeratorInternal())
			{
				co_yield op(val);
			}
		}

		template<typename Self, typename Func> requires IsFunc<Func, bool, const T&>
		Enumerator<T> Where(this Self&& self, Func&& predicate)
		{
			for (T& val : self.GetEnumeratorInternal())
			{
				if (predicate(val))
				{
					co_yield val;
				}
			}
		}

    private:
        template<typename R>
        static constexpr bool AlwaysFalse = false;

        template<typename Self>
        Enumerator<T> GetEnumeratorInternal(this Self&& self)
        {
			using Type = std::remove_reference_t<Self>;
			if constexpr (Internal::UserDefinedEnumerable<Type, T>)
			{
				return self.GetEnumerator();
			}
            if constexpr (std::ranges::range<Type>)
            {
                return self.GetEnumeratorForRange();
            }
            else
            {
                static_assert(AlwaysFalse<Type>, "Must define GetEnumerator() override if implementation does not satisfy std::ranges::range concept.");
            }
        }

        template<std::ranges::range Self>
        Enumerator<T> GetEnumeratorForRange(this Self&& self)
        {
            for (auto&& val : self)
            {
                co_yield val;
            }
        }
    };


	template<typename T>
	class Enumerator : public IEnumerable<T>
	{
	public:
		using promise_type = Internal::EnumeratorPromise<T>;
		using iterator = Internal::EnumeratorIterator<T>;
		using sentinel = Internal::EnumeratorSentinel;

	public:
		using CoroutineHandle = std::coroutine_handle<promise_type>; 

	public: // Make this an enumerable for chaining LINQ queries
		Enumerator<T> GetEnumerator() override { return std::move(*this); }

	public:
		Enumerator() noexcept = default;
		Enumerator(const Enumerator&) = delete;
		Enumerator(Enumerator&& other) : mHandle(std::exchange(other.mHandle, nullptr)) {}

		auto operator=(const Enumerator&) = delete;
		auto operator=(Enumerator && other)
		{
			if (std::addressof(other) != this)
			{
				if (mHandle)
					mHandle.destroy();

				mHandle = std::exchange(other.mHandle, nullptr);
			}

			return *this;
		}

		~Enumerator()
		{
			if (!mHandle)
				return;

			mHandle.destroy();
		}

		auto begin() -> iterator
		{
			iterator it{ mHandle };
			if (mHandle)
			{
				it++;
			}
			return it;
		}
		auto end() noexcept -> sentinel { return {}; }


	private:
		CoroutineHandle mHandle = nullptr;

		template<typename T>
		friend class Internal::EnumeratorPromise;

		Enumerator(CoroutineHandle handle) noexcept : mHandle(handle) {}
	};

	template<typename TReturn>
	auto Internal::EnumeratorPromise<TReturn>::get_return_object() noexcept -> Enumerator<TReturn>
	{
		return Enumerator<TReturn>(Enumerator<TReturn>::CoroutineHandle::from_promise(*this));
	}
}
#pragma once

#include "Internal/EnumerableInternal.h"

#include <ranges>
#include <iostream>

namespace slc {

	template<typename T>
	class IEnumerable;

	template<typename T>
	class Enumerator;

	template<typename T, size_t TSize>
	class Array;

	template<typename T, typename Allocator>
	class Vector;

	template<typename TKey, typename TValue,
		typename Hash,
		typename KeyEqual,
		typename Allocator>
	class Dictionary;

	template<typename T, typename R>
	concept Enumerable = std::derived_from<std::remove_reference_t<T>, IEnumerable<R>>;

	class IEnumerableBase
	{
	public:
		virtual ~IEnumerableBase() = 0;

		template<typename T>
		Enumerator<T> AsEnumerable() { return dynamic_cast<IEnumerable<T>*>(this)->GetEnumeratorImpl(); }

		template<Numeric StartType>
		static Enumerator<StartType> Range(StartType start, UNumeric auto count)
		{
			// Would go out of range, iterate to max value
			if (start > Limits<StartType>::Max - (count - 1)) [[unlikely]]
			{
				count = Limits<StartType>::Max - start + 1;
			}

			for (StartType i = start; i < start + count; i++)
			{
				co_yield i;
			}
		}
	};

	inline IEnumerableBase::~IEnumerableBase() {}

	/// <summary>
	/// Provides support for lazy evaluation of an enumerable sequence and a C# IEnumerable style interface. The first element of the 
	/// enumerable is evaluated eagerly to ensure the lifetime of temporary enumerable objects is maintained on chained functions.
	/// If the derived type T satisfies std::ranges::range&lt;T&gt; then use the MAKE_RANGE_ENUMERABLE(T) macro to override GetEnumerator(). 
	/// Otherwise, provide an override for GetEnumerator() that yields each item in the enumeration in sequence.
	/// </summary>
	/// <typeparam name="T">The type returned on each evaluation of the enumeration.</typeparam>
	template<typename T>
	class IEnumerable : public IEnumerableBase
	{
	public:
		using BaseType = IEnumerable<T>;
		using EnumeratorType = Enumerator<T>;

	public:
		virtual ~IEnumerable() {}
		virtual EnumeratorType GetEnumerator() = 0;

		template<typename Self>
		auto begin(this Self&& self) -> decltype(auto)
		{
			return std::forward<Self>(self).GetEnumeratorImpl().begin();
		}
		template<typename Self>
		auto end(this Self&& self) { return EnumeratorType::sentinel(); }

	protected:
		template<std::ranges::range Self>
		EnumeratorType GetEnumeratorForRange(this Self&& self)
		{
			for (auto&& val : std::forward<Self>(self))
			{
				co_yield val;
			}
		}
#define MAKE_RANGE_ENUMERABLE(...)  \
		EXPAND_TEMPLATE(__VA_ARGS__)::EnumeratorType GetEnumerator() override { return this->GetEnumeratorForRange(); } 

	private:
		template<typename R>
		static constexpr bool AlwaysFalse = false;

		friend class IEnumerableBase;

		template<Enumerable<T> Self>
		EnumeratorType GetEnumeratorImpl(this Self&& self)
		{
			// If possible directly dispatch correct function at compile time rather than 
			// resorting to virtual function. This is possible when method is called on
			// a derived type and type information can be determined from Self.

			using Type = std::remove_reference_t<Self>;
			if constexpr (std::same_as<Type, EnumeratorType>)
			{
				// Enumerator type is itself an IEnumerable so it can be used to chain functions.
				// Move it along the chain so that when the destructor is called on the temporary
				// Enumerator the coroutine is not destroyed.
				return std::move(self);
			}
			else if constexpr (std::ranges::range<Type>)
			{
				// Use std::ranges::range concept to get Enumerator
				return std::forward<Self>(self).GetEnumeratorForRange();
			}
			else
			{
				// Use overriden GetEnumerator method. Used for user defined types or when called from IEnumerableBase
				return std::forward<Self>(self).GetEnumerator();
			}
		}

		// C# LINQ-like functions
	public:
		template<typename Self, IsFunc<T, const T&, const T&> Func>
		T Aggregate(this Self&& self, Func&& func)
		{
			auto&& enumerator = std::forward<Self>(self).GetEnumeratorImpl();

			auto it = enumerator.begin();
			T result = *it++;

			while (it != enumerator.end())
			{
				result = func(result, *it++);
			}

			return result;
		}

		template<typename Self, IsPredicate<const T&> Func>
		bool All(this Self&& self, Func&& predicate)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				if (!predicate(val))
					return false;
			}

			return true;
		}

		template<typename Self>
		bool Any(this Self&& self)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				return true;
			}

			return false;
		}

		template<typename Self, IsPredicate<const T&> Func>
		bool Any(this Self&& self, Func&& predicate)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				if (predicate(val))
					return true;
			}

			return false;
		}

		template<typename Self>
		EnumeratorType Append(this Self&& self, T&& newVal)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				co_yield val;
			}
			co_yield newVal;
		}


		template<typename R, typename Self> requires Castable<T&, R&> or Castable<T, R>
		Enumerator<R> Cast(this Self&& self)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				co_yield std::forward<R>(val);
			}
		}

		template<typename Self, typename Other> requires std::derived_from<Other, IEnumerable<T>> and std::equality_comparable<T>
		EnumeratorType Concat(this Self&& self, Other&& other)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				co_yield val;
			}
			for (auto&& val : std::forward<Other>(other).GetEnumeratorImpl())
			{
				co_yield val;
			}
		}

		template<typename Self> requires std::equality_comparable<T>
		bool Contains(this Self&& self, const T& value)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				if (val == value)
					return true;
			}

			return false;
		}

		template<typename Self>
		size_t Count(this Self&& self)
		{
			size_t i = 0;
			if (self.TryGetNonEnumeratedCount(i))
			{
				return i;
			}

			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				i++;
			}

			return i;
		}

		template<typename Self, IsPredicate<const T&> Func>
		size_t Count(this Self&& self, Func&& predicate)
		{
			size_t i = 0;
			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				if (predicate(val))
				{
					i++;
				}
			}

			return i;
		}

		template<typename Self>
		bool Empty(this Self&& self)
		{
			return !std::forward<Self>(self).Any();
		}

		template<typename Self>
		Enumerator<std::tuple<size_t, T>> Enumerate(this Self&& self)
		{
			size_t i = 0;
			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				co_yield std::make_tuple(i++, val);
			}
		}

		template<typename Self>
		auto First(this Self&& self) -> decltype(auto)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				return val;
			}
		}

		template<typename Self> requires std::is_default_constructible_v<Self>
		T FirstOrDefault(this Self&& self)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				return val;
			}

			return T();
		}

		template<typename Self>
		T& Last(this Self&& self)
		{
			T* valPtr = nullptr;
			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				valPtr = std::addressof(val);
			}

			return *valPtr;
		}

		template<typename Self> requires std::is_default_constructible_v<Self> and std::is_copy_constructible_v<Self>
		T LastOrDefault(this Self&& self)
		{
			T* valPtr = nullptr;

			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				valPtr = std::addressof(val);
			}

			if (!valPtr)
				return T();

			return *valPtr;
		}

		template<typename Self> requires ComparableGreater<T>
		T& Max(this Self&& self)
		{
			T* valPtr = nullptr;
			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				if (!valPtr || std::greater<T>(val, *valPtr))
				{
					valPtr = std::addressof(val);
				}
			}

			return *valPtr;
		}

		template<typename Self> requires ComparableLess<T>
		T& Min(this Self&& self)
		{
			T* valPtr = nullptr;
			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				if (!valPtr || std::less<T>(val, *valPtr))
				{
					valPtr = std::addressof(val);
				}
			}

			return *valPtr;
		}

		template<typename Self>
		EnumeratorType Prepend(this Self&& self, T&& newVal)
		{
			co_yield std::forward<T>(newVal);
			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				co_yield val;
			}
		}

		template<typename Self>
		Enumerator<T> Reverse(this Self&& self)
		{
			using StoragePointerType = std::conditional_t<std::is_const_v<Self>, const T*, T*>;
			std::vector<StoragePointerType> tmp;

			size_t size;
			if (self.TryGetNonEnumeratedCount(size))
			{
				tmp.reserve(size);
			}

			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				tmp.emplace_back(&val);
			}

			for (auto it = tmp.rbegin(); it != tmp.rend(); ++it)
			{
				co_yield **it;
			}
		}

		template<typename R, typename Self, typename Func> requires IsFunc<Func, R, const T&>
		Enumerator<R> Select(this Self&& self, Func&& op)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				co_yield op(val);
			}
		}

		template<typename R, typename Self, typename Func> requires IsFunc<Func, Enumerator<R>, T&>
		Enumerator<R> SelectMany(this Self&& self, Func&& selector)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				Enumerator<R> valEnumerable = selector(val);
				for (auto&& val : valEnumerable.GetEnumerator())
				{
					co_yield val;
				}
			}
		}

		template<typename Self, Numeric Count>
		EnumeratorType Skip(this Self&& self, Count count)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				if (count > 0)
				{
					count--;
				}
				else
				{
					co_yield val;
				}
			}
		}

		template<typename Self, IsPredicate<const T&> Func> 
		EnumeratorType SkipWhile(this Self&& self, Func&& predicate)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				if (!predicate(val))
				{
					co_yield val;
				}
			}
		}

		template<typename Self> requires Summable<T>
		T Sum(this Self&& self)
		{
			T result;
			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				if constexpr (AddAssignable<T>)
				{
					result += val;
				}
				else if constexpr (UnaryAddable<T>)
				{
					result = result + val;
				}
			}
			return result;
		}


		template<typename Self> 
		EnumeratorType Take(this Self&& self, Numeric auto count)
		{
			if (count <= 0)
			{
				co_return;
			}

			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				if (count-- == 0)
					co_return;

				co_yield val;
			}
		}

		template<typename Self, IsPredicate<const T&> Func>
		EnumeratorType TakeWhile(this Self&& self, Func&& predicate)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				if (predicate(val))
				{
					co_yield val;
				}
			}
		}

		template<typename TKey, typename TValue, typename Self,
			typename Hash = std::hash<TKey>,
			typename KeyEqual = std::equal_to<TKey>,
			typename Allocator = std::allocator<std::pair<const TKey, TValue>>
		>
		Dictionary<TKey, TValue, Hash, KeyEqual, Allocator> ToDictionary(this Self&& self) requires std::convertible_to<T, std::pair<TKey, TValue>>
		{
			Dictionary<TKey, TValue, Hash, KeyEqual, Allocator> result;
			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				result.emplace(std::forward<std::pair<TKey, TValue>>(val));
			}
			return result;
		}

		template<typename Self, typename Allocator = std::allocator<T>>
		Vector<T, Allocator> ToVector(this Self&& self)
		{
			size_t count = self.Count();

			Vector<T, Allocator> result;
			result.reserve(count);

			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				result.emplace_back(std::move(val));
			}
			return result;
		}

		template<size_t Size, typename Self>
		Array<T, Size> ToArray(this Self&& self)
		{
			Array<const T*, Size> result;
			for (auto&& [i, val] : std::forward<Self>(self).Enumerate().Take(Size))
			{
				result[i] = std::addressof(val);
			}

			return ArrayConversion(result);
		}

		template<typename Self>
		bool TryGetNonEnumeratedCount(this Self&& self, size_t& count)
		{
			if constexpr (Sizeable<Self>)
			{
				count = self.size();
				return true;
			}
			return false;
		}

		template<typename Self, typename Func> requires IsFunc<Func, bool, const T&>
		EnumeratorType Where(this Self&& self, Func&& predicate)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorImpl())
			{
				if (predicate(val))
				{
					co_yield val;
				}
			}
		}

		template<typename Self, typename TOut, typename Other, typename TIn, IsFunc<TOut, const T&, const TIn&> Func> requires std::derived_from<Other, IEnumerable<TIn>>
		Enumerator<TOut> Zip(this Self&& self, const Other& other, Func&& resultSelector)
		{
			auto&& firstEnumerator = std::forward<Self>(self).GetEnumeratorImpl();
			auto&& secondEnumerator = other.GetEnumeratorImpl();

			auto firstIt = firstEnumerator.begin();
			auto secondIt = secondEnumerator.begin();

			while (firstIt != firstEnumerator.end() && secondIt != secondEnumerator.end())
			{
				co_yield resultSelector(*firstIt, *secondIt);

				firstIt++;
				secondIt++;
			}
		}

		template<typename Self, typename TOther, typename Other> requires std::derived_from<Other, IEnumerable<TOther>>
		Enumerator<std::tuple<T&, TOther&>> Zip(this Self&& self, const Other& other)
		{
			auto&& firstEnumerator = std::forward<Self>(self).GetEnumeratorImpl();
			auto&& secondEnumerator = other.GetEnumeratorImpl();

			auto firstIt = firstEnumerator.begin();
			auto secondIt = secondEnumerator.begin();

			while (firstIt != firstEnumerator.end() && secondIt != secondEnumerator.end())
			{
				co_yield std::make_tuple(*firstIt, *secondIt);

				firstIt++;
				secondIt++;
			}
		}

	public:
		template<Numeric Count>
		static Enumerator<T> Repeat(const T& val, Count count) requires std::is_copy_constructible_v<T>
		{
			if (count <= 0)
				co_return;

			for (Count i = 0; i < count; i++)
			{
				co_yield T(val);
			}
		}


	private:
		template<std::size_t N>
		static Array<T, N> ArrayConversion(const Array<const T*, N>& values)
		{
			return ArrayConversionInternal(values, std::make_index_sequence<N>());
		}

		template<std::size_t N, std::size_t... Is>
		static std::array<T, N> ArrayConversionInternal(const Array<const T*, N>& values, std::index_sequence<Is...>)
		{
			return { { CopyOrDefault(values[Is])... } };
		}

		static T CopyOrDefault(const T* ptr) { return ptr ? T(*ptr) : T(); }
	};

	template<typename T>
	class Enumerator : public IEnumerable<T>
	{
	public:
		using promise_type = Internal::EnumeratorPromise<T>;
		using iterator = Internal::EnumeratorIterator<T>;
		using sentinel = Internal::EnumeratorSentinel;
		using const_iterator = Internal::EnumeratorIterator<const T>;

	public:
		using CoroutineHandle = std::coroutine_handle<promise_type>;
		using EnumeratorType = IEnumerable<T>::EnumeratorType;

		EnumeratorType GetEnumerator() override { return std::move(*this); }

	public:
		Enumerator() noexcept = default;
		Enumerator(const Enumerator&) = delete;
		Enumerator(Enumerator&& other) noexcept : mHandle(std::exchange(other.mHandle, nullptr)) {}

		auto operator=(const Enumerator&) = delete;
		auto operator=(Enumerator&& other)
		{
			if (std::addressof(other) != this)
			{
				if (mHandle)
					mHandle.destroy();

				mHandle = std::exchange(other.mHandle, nullptr);
			}

			return *this;
		}

		~Enumerator() override
		{
			if (!mHandle)
				return;

			mHandle.destroy();
		}

		auto begin() -> iterator
		{
			iterator it{ mHandle };
			return it;
		}
		auto begin() const -> const_iterator
		{
			const_iterator it{ mHandle };
			return it;
		}

		auto end() const noexcept -> sentinel { return {}; }


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
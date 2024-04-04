#pragma once

#include "Internal/EnumerableInternal.h"

#include <ranges>
#include <iostream>

namespace slc {

	template<typename T>
	class Enumerator;

	template<typename T>
	class IEnumerable;

	template<typename T, typename R>
	concept Enumerable = std::ranges::range<T> or Internal::UserDefinedEnumerable<T, R> or std::same_as<std::remove_reference_t<T>, IEnumerable<R>>;

	class IEnumerableBase
	{
	public:
		virtual ~IEnumerableBase() = 0;

		template<typename T>
		Enumerator<T> AsEnumerable() { return dynamic_cast<IEnumerable<T>*>(this)->GetEnumeratorInternal(); }
	};

	inline IEnumerableBase::~IEnumerableBase() {}

	template<typename T>
	class IEnumerable : public IEnumerableBase
	{
	public:
		using BaseType = IEnumerable<T>;
		using EnumeratorType = Enumerator<T>;

	public:
		virtual ~IEnumerable() = 0;

	public:
		virtual EnumeratorType GetEnumerator() = 0;

		// C# LINQ-like functions
	public:
		template<typename Self, IsFunc<T, const T&, const T&> Func>
		T Aggregate(this Self&& self, Func&& func)
		{
			T result = self.First();
			for (auto&& val : std::forward<Self>(self).GetEnumeratorInternal())
			{
				result = func(result, std::forward<T>(val));
			}

			return result;
		}

		template<typename Self, IsPredicate<const T&> Func>
		bool All(this Self&& self, Func&& predicate)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorInternal())
			{
				if (!predicate(std::forward<T>(val)))
					return false;
			}

			return true;
		}

		template<typename Self>
		bool Any(this Self&& self)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorInternal())
			{
				return true;
			}

			return false;
		}

		template<typename Self, IsPredicate<const T&> Func>
		bool Any(this Self&& self, Func&& predicate)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorInternal())
			{
				if (predicate(std::forward<T>(val)))
					return true;
			}

			return false;
		}

		template<typename Self>
		EnumeratorType Append(this Self&& self, T&& newVal)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorInternal())
			{
				co_yield std::forward<T>(val);
			}
			co_yield std::forward<T>(newVal);
		}


		template<typename R, typename Self> requires Castable<T&, R&> or Castable<T, R>
		Enumerator<R> Cast(this Self&& self)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorInternal())
			{
				co_yield std::forward<R>(val);
			}
		}

		template<typename Self, typename Other> requires std::derived_from<Other, IEnumerable<T>> and std::equality_comparable<T>
		EnumeratorType Concat(this Self&& self, Other&& other)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorInternal())
			{
				co_yield std::forward<T>(val);
			}
			for (auto&& val : std::forward<Other>(other).GetEnumeratorInternal())
			{
				co_yield std::forward<T>(val);
			}
		}

		template<typename Self> requires std::equality_comparable<T>
		bool Contains(this Self&& self, const T& value)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorInternal())
			{
				if (std::forward<T>(val) == value)
					return true;
			}

			return false;
		}

		template<typename Self>
		int Count(this Self&& self)
		{
			int i = 0;
			for (auto&& val : std::forward<Self>(self).GetEnumeratorInternal())
			{
				i++;
			}

			return i;
		}

		template<typename Self, IsPredicate<const T&> Func>
		int Count(this Self&& self, Func&& predicate)
		{
			int i = 0;
			for (auto&& val : std::forward<Self>(self).GetEnumeratorInternal())
			{
				if (predicate(std::forward<T>(val)))
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

		template<typename Self, typename Other> requires std::derived_from<Other, IEnumerable<T>> and std::equality_comparable<T>
		EnumeratorType Except(this Self&& self, Other&& other) 
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorInternal())
			{
				if (!other.Contains(val))
				{
					co_yield std::forward<T>(val);
				}
			}
		}

		template<typename Self>
		auto First(this Self&& self) -> decltype(auto)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorInternal())
			{
				return std::forward<T>(val);
			}
		}

		template<typename Self> requires std::is_default_constructible_v<Self>
		T FirstOrDefault(this Self&& self)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorInternal())
			{
				return std::forward<T>(val);
			}

			return T();
		}

		template<typename Self>
		T& Last(this Self&& self)
		{
			T* valPtr = nullptr;
			for (auto&& val : std::forward<Self>(self).GetEnumeratorInternal())
			{
				valPtr = std::addressof(val);
			}

			return *valPtr;
		}

		template<typename Self> requires std::is_default_constructible_v<Self> and std::is_copy_constructible_v<Self>
		T LastOrDefault(this Self&& self)
		{
			T* valPtr = nullptr;

			for (auto&& val : std::forward<Self>(self).GetEnumeratorInternal())
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
			for (auto&& val : std::forward<Self>(self).GetEnumeratorInternal())
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
			for (auto&& val : std::forward<Self>(self).GetEnumeratorInternal())
			{
				if (!valPtr || std::less<T>(val, *valPtr))
				{
					valPtr = std::addressof(val);
				}
			}

			return *valPtr;
		}

		template<typename R, typename Self, typename Func> requires IsFunc<Func, R, T&>
		Enumerator<R> Select(this Self&& self, Func&& op)
		{
			for (auto&& val : std::forward<Self>(self).GetEnumeratorInternal())
			{
				co_yield op(std::forward<T>(val));
			}
		}

		template<typename Self, typename Func> requires IsFunc<Func, bool, const T&>
		EnumeratorType Where(this Self&& self, Func&& predicate)
		{
			if constexpr (std::is_rvalue_reference_v<Self>)
			{
				int a = 0;
			}

			for (auto&& val : std::forward<Self>(self).GetEnumeratorInternal())
			{
				if (predicate(val))
				{
					co_yield std::forward<T>(val);
				}
			}
		}

	protected:
		template<std::ranges::range Self>
		EnumeratorType GetEnumeratorForRange(this Self&& self)
		{
			for (auto&& val : std::forward<Self>(self))
			{
				co_yield std::forward<T>(val);
			}
		}

	private:
		template<typename R>
		static constexpr bool AlwaysFalse = false;

		friend class IEnumerableBase;

		template<Enumerable<T> Self>
		EnumeratorType GetEnumeratorInternal(this Self&& self)
		{
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
				return self.GetEnumeratorForRange();
			}
			else if constexpr (Internal::UserDefinedEnumerable<Type, T> or std::same_as<Type, IEnumerable<T>>)
			{
				// Use overriden GetEnumerator method. If not overriden enumeration returns no results.
				return self.GetEnumerator();
			}
		}
	};

	template<typename T>
	inline IEnumerable<T>::~IEnumerable<T>() {}

	template<typename T>
	class Enumerator : public IEnumerable<T>
	{
	public:
		using promise_type = Internal::EnumeratorPromise<T>;
		using iterator = Internal::EnumeratorIterator<T>;
		using sentinel = Internal::EnumeratorSentinel;

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

		auto begin() const -> iterator
		{
			iterator it{ mHandle };
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
#pragma once

#include "SL/Core/Common/Base.h"

namespace sl {

	enum class OverflowPolicy
	{
		Throw,
		NoOp
	};

	template < typename T, std::size_t N, OverflowPolicy Policy = OverflowPolicy::Throw >
	class SmallVector
	{
	public:
		using value_type = T;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference = T&;
		using const_reference = T const&;
		using pointer = T*;
		using const_pointer = T const*;

	private:
		static constexpr size_type CapacityValue = N;

		alignas( T ) Byte mBuffer[ sizeof( T ) * N ]{};
		size_type mSize{};

	public:
		SmallVector() noexcept = default;

		SmallVector( SmallVector const& other )
		{
			for ( size_type i = 0; i < other.mSize; ++i )
				emplace_back( other[ i ] );
		}

		SmallVector( SmallVector&& other ) noexcept( std::is_nothrow_move_constructible_v< T > )
		{
			for ( size_type i = 0; i < other.mSize; ++i )
				emplace_back( std::move( other[ i ] ) );
			other.clear();
		}

		SmallVector& operator=( SmallVector const& other )
		{
			if ( this == &other )
				return *this;

			clear();
			for ( size_type i = 0; i < other.mSize; ++i )
				emplace_back( other[ i ] );
			return *this;
		}

		SmallVector& operator=( SmallVector&& other ) noexcept(
			std::is_nothrow_move_constructible_v< T > && std::is_nothrow_move_assignable_v< T >
		)
		{
			if ( this == &other )
				return *this;

			clear();
			for ( size_type i = 0; i < other.mSize; ++i )
				emplace_back( std::move( other[ i ] ) );
			other.clear();
			return *this;
		}

		~SmallVector()
		{
			DestroyAll();
		}

		// ---- capacity ----

		static constexpr size_type capacity() noexcept
		{
			return CapacityValue;
		}
		size_type size() const noexcept
		{
			return mSize;
		}
		bool empty() const noexcept
		{
			return mSize == 0;
		}

		// ---- element access ----

		reference operator[]( size_type i ) noexcept
		{
			return *PtrAt( i );
		}
		const_reference operator[]( size_type i ) const noexcept
		{
			return *PtrAt( i );
		}

		reference at( size_type i )
		{
			if ( i >= mSize )
				throw std::out_of_range( "SmallVector::at out of range" );
			return ( *this )[ i ];
		}

		const_reference at( size_type i ) const
		{
			if ( i >= mSize )
				throw std::out_of_range( "SmallVector::at out of range" );
			return ( *this )[ i ];
		}

		reference front() noexcept
		{
			return ( *this )[ 0 ];
		}
		const_reference front() const noexcept
		{
			return ( *this )[ 0 ];
		}

		reference back() noexcept
		{
			return ( *this )[ mSize - 1 ];
		}
		const_reference back() const noexcept
		{
			return ( *this )[ mSize - 1 ];
		}

		pointer data() noexcept
		{
			return PtrAt( 0 );
		}
		const_pointer data() const noexcept
		{
			return PtrAt( 0 );
		}

		// ---- modifiers ----

		void clear() noexcept
		{
			DestroyAll();
		}

		void pop_back() noexcept
		{
			if ( mSize == 0 )
				return;

			PtrAt( mSize - 1 )->~T();
			--mSize;
		}

		void push_back( T const& v )
		{
			emplace_back( v );
		}
		void push_back( T&& v )
		{
			emplace_back( std::move( v ) );
		}

		template < typename... Args >
		reference emplace_back( Args&&... args )
		{
			if ( mSize < N )
			{
				ConstructAt( mSize, std::forward< Args >( args )... );
				++mSize;
				return back();
			}

			// full
			if constexpr ( Policy == OverflowPolicy::Throw )
			{
				throw std::length_error( "SmallVector capacity exceeded" );
			}
			else // NoOp
			{
				// do nothing; best-effort reference return
				return back();
			}
		}

		// ---- iterators ----

		class iterator
		{
		public:
			using iterator_category = std::random_access_iterator_tag;
			using value_type = T;
			using difference_type = std::ptrdiff_t;
			using pointer = T*;
			using reference = T&;

			iterator() noexcept = default;

			reference operator*() const noexcept
			{
				return ( *mVec )[ mIndex ];
			}
			pointer operator->() const noexcept
			{
				return &( ( *mVec )[ mIndex ] );
			}

			iterator& operator++() noexcept
			{
				++mIndex;
				return *this;
			}
			iterator operator++( int ) noexcept
			{
				auto t = *this;
				++*this;
				return t;
			}

			iterator& operator--() noexcept
			{
				--mIndex;
				return *this;
			}
			iterator operator--( int ) noexcept
			{
				auto t = *this;
				--*this;
				return t;
			}

			iterator& operator+=( difference_type d ) noexcept
			{
				mIndex += static_cast< size_type >( d );
				return *this;
			}
			iterator& operator-=( difference_type d ) noexcept
			{
				mIndex -= static_cast< size_type >( d );
				return *this;
			}

			friend iterator operator+( iterator it, difference_type d ) noexcept
			{
				it += d;
				return it;
			}
			friend iterator operator+( difference_type d, iterator it ) noexcept
			{
				it += d;
				return it;
			}
			friend iterator operator-( iterator it, difference_type d ) noexcept
			{
				it -= d;
				return it;
			}
			friend difference_type operator-( iterator a, iterator b ) noexcept
			{
				return static_cast< difference_type >( a.mIndex ) - static_cast< difference_type >( b.mIndex );
			}

			reference operator[]( difference_type d ) const noexcept
			{
				return ( *mVec )[ mIndex + static_cast< size_type >( d ) ];
			}

			friend bool operator==( iterator a, iterator b ) noexcept
			{
				return a.mVec == b.mVec && a.mIndex == b.mIndex;
			}
			friend bool operator!=( iterator a, iterator b ) noexcept
			{
				return !( a == b );
			}
			friend bool operator<( iterator a, iterator b ) noexcept
			{
				return a.mIndex < b.mIndex;
			}
			friend bool operator>( iterator a, iterator b ) noexcept
			{
				return b < a;
			}
			friend bool operator<=( iterator a, iterator b ) noexcept
			{
				return !( b < a );
			}
			friend bool operator>=( iterator a, iterator b ) noexcept
			{
				return !( a < b );
			}

		private:
			friend class SmallVector;
			SmallVector* mVec{};
			size_type mIndex{};

			iterator( SmallVector* v, size_type i ) noexcept
				: mVec( v ), mIndex( i )
			{}
		};

		class const_iterator
		{
		public:
			using iterator_category = std::random_access_iterator_tag;
			using value_type = T const;
			using difference_type = std::ptrdiff_t;
			using pointer = T const*;
			using reference = T const&;

			const_iterator() noexcept = default;
			const_iterator( iterator it ) noexcept
				: mVec( it.mVec ), mIndex( it.mIndex )
			{}

			reference operator*() const noexcept
			{
				return ( *mVec )[ mIndex ];
			}
			pointer operator->() const noexcept
			{
				return &( ( *mVec )[ mIndex ] );
			}

			const_iterator& operator++() noexcept
			{
				++mIndex;
				return *this;
			}
			const_iterator operator++( int ) noexcept
			{
				auto t = *this;
				++*this;
				return t;
			}

			const_iterator& operator--() noexcept
			{
				--mIndex;
				return *this;
			}
			const_iterator operator--( int ) noexcept
			{
				auto t = *this;
				--*this;
				return t;
			}

			const_iterator& operator+=( difference_type d ) noexcept
			{
				mIndex += static_cast< size_type >( d );
				return *this;
			}
			const_iterator& operator-=( difference_type d ) noexcept
			{
				mIndex -= static_cast< size_type >( d );
				return *this;
			}

			friend const_iterator operator+( const_iterator it, difference_type d ) noexcept
			{
				it += d;
				return it;
			}
			friend const_iterator operator+( difference_type d, const_iterator it ) noexcept
			{
				it += d;
				return it;
			}
			friend const_iterator operator-( const_iterator it, difference_type d ) noexcept
			{
				it -= d;
				return it;
			}
			friend difference_type operator-( const_iterator a, const_iterator b ) noexcept
			{
				return static_cast< difference_type >( a.mIndex ) - static_cast< difference_type >( b.mIndex );
			}

			reference operator[]( difference_type d ) const noexcept
			{
				return ( *mVec )[ mIndex + static_cast< size_type >( d ) ];
			}

			friend bool operator==( const_iterator a, const_iterator b ) noexcept
			{
				return a.mVec == b.mVec && a.mIndex == b.mIndex;
			}
			friend bool operator!=( const_iterator a, const_iterator b ) noexcept
			{
				return !( a == b );
			}
			friend bool operator<( const_iterator a, const_iterator b ) noexcept
			{
				return a.mIndex < b.mIndex;
			}
			friend bool operator>( const_iterator a, const_iterator b ) noexcept
			{
				return b < a;
			}
			friend bool operator<=( const_iterator a, const_iterator b ) noexcept
			{
				return !( b < a );
			}
			friend bool operator>=( const_iterator a, const_iterator b ) noexcept
			{
				return !( a < b );
			}

		private:
			friend class SmallVector;
			SmallVector const* mVec{};
			size_type mIndex{};

			const_iterator( SmallVector const* v, size_type i ) noexcept
				: mVec( v ), mIndex( i )
			{}
		};

		iterator begin() noexcept
		{
			return iterator( this, 0 );
		}
		iterator end() noexcept
		{
			return iterator( this, mSize );
		}

		const_iterator begin() const noexcept
		{
			return const_iterator( this, 0 );
		}
		const_iterator end() const noexcept
		{
			return const_iterator( this, mSize );
		}

		const_iterator cbegin() const noexcept
		{
			return const_iterator( this, 0 );
		}
		const_iterator cend() const noexcept
		{
			return const_iterator( this, mSize );
		}

	private:
		pointer PtrAt( size_type i ) noexcept
		{
			auto* p = reinterpret_cast< pointer >( mBuffer + i * sizeof( T ) );
			return std::launder( p );
		}

		const_pointer PtrAt( size_type i ) const noexcept
		{
			auto* p = reinterpret_cast< const_pointer >( mBuffer + i * sizeof( T ) );
			return std::launder( p );
		}

		void DestroyAll() noexcept
		{
			for ( size_type i = 0; i < mSize; ++i )
				PtrAt( i )->~T();
			mSize = 0;
		}

		template < typename... Args >
		void ConstructAt( size_type i, Args&&... args )
		{
			::new ( static_cast< void* >( mBuffer + i * sizeof( T ) ) )
				T( std::forward< Args >( args )... );
		}
	};

} // namespace sl
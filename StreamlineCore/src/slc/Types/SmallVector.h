#pragma once

#include "slc/Common/Base.h"

namespace slc {

	namespace detail {

		template < typename Vector, bool IsConst >
		class SmallVectorIterator
		{
		public:
			using difference_type = typename Vector::difference_type;
			using value_type = std::conditional_t< IsConst, const typename Vector::value_type, typename Vector::value_type >;
			using pointer = std::conditional_t< IsConst, typename Vector::const_pointer, typename Vector::pointer >;
			using reference = std::conditional_t< IsConst, typename Vector::const_reference, typename Vector::reference >;

			using iterator_category = std::contiguous_iterator_tag;

		public:
			SmallVectorIterator( pointer heap )
				: mPtr( heap )
			{}

			SmallVectorIterator& operator++()
			{
				mPtr++;
				return *this;
			}
			SmallVectorIterator operator++( int )
			{
				SmallVectorIterator temp = *this;
				temp++;
				return temp;
			}

			SmallVectorIterator& operator--()
			{
				mPtr--;
				return *this;
			}
			SmallVectorIterator operator--( int )
			{
				SmallVectorIterator temp = *this;
				temp--;
				return temp;
			}

			SmallVectorIterator& operator+=( difference_type offset )
			{
				mPtr += offset;
				return *this;
			}
			SmallVectorIterator operator+( difference_type offset ) const
			{
				SmallVectorIterator it = *this;
				it += offset;
				return it;
			}
			SmallVectorIterator& operator-=( difference_type offset )
			{
				return *this += -offset;
			}
			SmallVectorIterator operator-( difference_type offset ) const
			{
				SmallVectorIterator it = *this;
				it += -offset;
				return it;
			}

			difference_type operator-( const SmallVectorIterator& rhs ) const
			{
				return mPtr - rhs.mPtr;
			}


			reference operator[]( difference_type index )
			{
				return *( mPtr + index );
			}

			pointer operator->()
			{
				return mPtr;
			}

			reference operator*()
			{
				return *mPtr;
			}

			bool operator==( const SmallVectorIterator& other ) const
			{
				return mPtr == other.mPtr;
			}
			bool operator!=( const SmallVectorIterator& other ) const
			{
				return !( *this == other );
			}

			operator pointer()
			{
				return mPtr;
			}

		private:
			pointer mPtr;
		};
	} // namespace detail

	template < typename T, typename Alloc = std::allocator< T > >
		requires std::is_default_constructible_v< T >
	class SmallVector
	{
	private:
		using AllocTy = typename std::allocator_traits< Alloc >::template rebind_alloc< T >;
		using AllocTraits = std::allocator_traits< AllocTy >;

	public:
		using value_type = T;
		using allocator_type = Alloc;
		using pointer = typename AllocTraits::pointer;
		using const_pointer = typename AllocTraits::const_pointer;
		using reference = T&;
		using const_reference = const T&;
		using size_type = typename AllocTy::size_type;
		using difference_type = typename AllocTy::difference_type;

		using iterator = detail::SmallVectorIterator< SmallVector, false >;
		using const_iterator = detail::SmallVectorIterator< SmallVector, true >;
		using reverse_iterator = std::reverse_iterator< iterator >;
		using const_reverse_iterator = std::reverse_iterator< const_iterator >;

	private:
		static constexpr size_type SmallBufferSize = 8;
		using array_type = std::array< value_type, SmallBufferSize >;

		static constexpr size_type ScaleFactor = 2;

		template < typename Self >
		using deduce_pointer = std::conditional_t< std::is_const_v< std::remove_reference_t< Self > >, const_pointer, pointer >;
		template < typename Self >
		using deduce_reference = std::conditional_t< std::is_const_v< std::remove_reference_t< Self > >, const_reference, reference >;

	public: // Constructors
		constexpr SmallVector() noexcept( std::is_nothrow_default_constructible_v< Alloc > ) = default;

		explicit constexpr SmallVector( Alloc const& alloc ) noexcept( std::is_nothrow_copy_constructible_v< Alloc > )
			: mAlloc( alloc ) {};

		explicit SmallVector( size_type count, Alloc const& alloc = Alloc() )
			: mAlloc( alloc )
		{
			ConstructData( count );
		}

		constexpr SmallVector( size_type count, T const& value, Alloc const& alloc = Alloc() )
			: mAlloc( alloc )
		{
			ConstructData( count, value );
		}

		constexpr SmallVector( SmallVector const& other )
			: mAlloc( other.mAlloc )
		{
			Allocate( other.mCapacity );
			other.CopyData( GetDataPointer() );
			mSize = other.mSize;
		}
		constexpr SmallVector( SmallVector&& other ) noexcept( std::is_nothrow_move_constructible_v< T > )
			: mAlloc( std::move( other.mAlloc ) )
			, mSize( other.mSize )
			, mCapacity( other.mCapacity )
			, mData( std::move( other.mData ) )
		{
			other.mData.none = {};
			other.mSize = 0;
			other.mCapacity = 0;
		}

		constexpr SmallVector( SmallVector const& other, std::type_identity_t< Alloc > const& alloc )
			: mData( other.mData )
			, mSize( other.mSize )
			, mCapacity( other.mCapacity )
			, mAlloc( alloc )
		{
		}

		constexpr SmallVector( SmallVector&& other, std::type_identity_t< Alloc > const& alloc ) noexcept( std::is_nothrow_move_constructible_v< T > )
			: mData( std::move( other.mData ) )
			, mSize( other.mSize )
			, mCapacity( other.mCapacity )
			, mAlloc( alloc )
		{
			other.mData.none = {};
			other.mSize = 0;
			other.mCapacity = 0;
		}

		~SmallVector()
		{
			Release();
		}


		SmallVector& operator=( SmallVector const& other )
		{
			if ( std::addressof( other ) == this )
				return *this;

			Release();

			mAlloc = other.mAlloc;
			Allocate( other.mCapacity );
			other.CopyData( GetDataPointer() );
			mSize = other.mSize;

			return *this;
		}

		SmallVector& operator=( SmallVector&& other ) noexcept( std::is_nothrow_move_assignable_v< T > )
		{
			if ( std::addressof( other ) == this )
				return *this;

			Release();

			mAlloc = std::move( other.mAlloc );
			mSize = other.mSize;
			mCapacity = other.mCapacity;
			mData = std::move( other.mData );

			other.mData.none = {};
			other.mSize = 0;
			other.mCapacity = 0;

			return *this;
		}

	public: // Iterators
		constexpr iterator begin() noexcept
		{
			return iterator( GetDataPointer() );
		}
		constexpr iterator end() noexcept
		{
			return iterator( GetDataPointer() + mSize );
		}
		constexpr const_iterator begin() const noexcept
		{
			return const_iterator( GetDataPointer() );
		}
		constexpr const_iterator end() const noexcept
		{
			return const_iterator( GetDataPointer() + mSize );
		}
		constexpr const_iterator cbegin() const noexcept
		{
			return const_iterator( GetDataPointer() );
		}
		constexpr const_iterator cend() const noexcept
		{
			return const_iterator( GetDataPointer() + mSize );
		}

		constexpr reverse_iterator rbegin() noexcept
		{
			return reverse_iterator( GetDataPointer() + mSize );
		}
		constexpr reverse_iterator rend() noexcept
		{
			return reverse_iterator( GetDataPointer() );
		}
		constexpr const_reverse_iterator rbegin() const noexcept
		{
			return const_reverse_iterator( GetDataPointer() + mSize );
		}
		constexpr const_reverse_iterator rend() const noexcept
		{
			return const_reverse_iterator( GetDataPointer() );
		}
		constexpr const_reverse_iterator crbegin() const noexcept
		{
			return const_reverse_iterator( GetDataPointer() + mSize );
		}
		constexpr const_reverse_iterator crend() const noexcept
		{
			return const_reverse_iterator( GetDataPointer() );
		}

	public: // Element Access
		template < typename Self >
		constexpr deduce_reference< Self > at( this Self&& self, size_type pos )
		{
			if ( pos >= self.mSize )
				throw std::out_of_range{};

			return *( std::forward< Self >( self ).GetDataPointer() + pos );
		}

		template < typename Self >
		constexpr deduce_reference< Self > operator[]( this Self&& self, size_type pos )
		{
			ASSERT( pos < self.mSize );
			return *( std::forward< Self >( self ).GetDataPointer() + pos );
		}

		template < typename Self >
		constexpr deduce_reference< Self > front( this Self&& self )
		{
			ASSERT( self.mSize > 0 );
			return *std::forward< Self >( self ).GetDataPointer();
		}

		template < typename Self >
		constexpr deduce_reference< Self > back( this Self&& self )
		{
			ASSERT( self.mSize > 0 );
			return *( std::forward< Self >( self ).GetDataPointer() + self.mSize - 1 );
		}

		template < typename Self >
		constexpr deduce_pointer< Self > data( this Self&& self )
		{
			return std::forward< Self >( self ).GetDataPointer();
		}

	public: // Capacity
		constexpr bool empty() const
		{
			return mSize == 0;
		}
		constexpr size_type size() const
		{
			return mSize;
		}
		constexpr size_type max_size() const
		{
			return Limits< size_type >::Max;
		}
		constexpr size_type capacity() const
		{
			return mCapacity;
		}

		void reserve( size_type new_cap )
		{
			if ( new_cap <= SmallBufferSize )
				return;

			if ( new_cap <= mCapacity )
				return;

			AllocateLarge( new_cap );
		}


	public: // Modifiers
		void clear()
		{
			DestroyData( 0, mSize );
			mSize = 0;
		}

		constexpr void push_back( const T& value )
		{
			DemandExtraCapacity( 1 );

			auto construct_ptr = GetDataPointer() + mSize++;
			std::construct_at( construct_ptr, value );
		}
		constexpr void push_back( T&& value )
		{
			DemandExtraCapacity( 1 );

			auto construct_ptr = GetDataPointer() + mSize++;
			std::construct_at( construct_ptr, std::move( value ) );
		}

		template < typename... Args >
			requires std::constructible_from< T, Args... >
		constexpr reference emplace_back( Args&&... args )
		{
			DemandExtraCapacity( 1 );

			auto construct_ptr = GetDataPointer() + mSize++;
			return *std::construct_at( construct_ptr, std::forward< Args >( args )... );
		}

		constexpr void pop_back()
		{
			auto destruct_ptr = GetDataPointer() + mSize--;
			std::destroy_at( destruct_ptr );
		}

		constexpr void resize( size_type count )
		{
			if ( count < mSize )
			{
				DestroyData( count, mSize );
			}
			else
			{
				ConstructData( count );
			}
		}
		constexpr void resize( size_type count, T const& value )
		{
			if ( count < mSize )
			{
				DestroyData( count, mSize );
			}
			else
			{
				ConstructData( count, value );
			}
		}

	private:
		constexpr void Allocate( size_type size )
		{
			if ( mCapacity >= size )
				return;

			if ( size <= SmallBufferSize )
				AllocateSmall( size );
			else
				AllocateLarge( size );
		}

		constexpr void Release()
		{
			if ( IsEmptyState() )
				return;

			auto data_ptr = GetDataPointer();
			ASSERT( data_ptr != nullptr );

			DestroyData( 0, mSize );

			if ( IsUsingLargeBuffer() )
			{
				mAlloc.deallocate( data_ptr, mCapacity );
			}

			mData.none = {};
			mSize = 0;
			mCapacity = 0;
		}


		constexpr void AllocateSmall( size_type new_size )
		{
			auto new_array = array_type{};

			MoveData( new_array.data() );

			mData.stack = std::move( new_array );
			mCapacity = SmallBufferSize;
		}

		constexpr void AllocateLarge( size_type new_size )
		{
			pointer new_ptr = mAlloc.allocate( new_size );

			MoveData( new_ptr );

			mData.heap = new_ptr;
			mCapacity = new_size;
		}

	private:
		constexpr void DemandExtraCapacity( size_type extra_elements )
		{
			auto new_size = mCapacity == 0 ? 1 : mCapacity;
			while ( mSize + extra_elements > new_size )
				new_size *= ScaleFactor;

			Allocate( new_size );
		}

		template < typename Self >
		constexpr deduce_pointer< Self > GetDataPointer( this Self&& self )
		{
			if ( self.IsEmptyState() )
				return nullptr;

			if ( self.IsUsingSmallBuffer() )
				return std::forward< Self >( self ).mData.stack.data();

			return std::forward< Self >( self ).mData.heap;
		}

		constexpr void MoveData( pointer new_ptr )
		{
			pointer move_ptr = GetDataPointer();

			// Move elements from previous memory
			for ( auto it_ptr = new_ptr; it_ptr != new_ptr + mSize; it_ptr++, move_ptr++ )
			{
				std::construct_at( it_ptr, std::move( *move_ptr ) );
			}
		}

		constexpr void CopyData( pointer new_ptr ) const
		{
			const_pointer copy_ptr = GetDataPointer();

			// Move elements from previous memory
			for ( auto it_ptr = new_ptr; it_ptr != new_ptr + mSize; it_ptr++, copy_ptr++ )
			{
				std::construct_at( it_ptr, *copy_ptr );
			}
		}

		template < typename... Args >
		constexpr void ConstructData( size_type count, Args&&... args )
		{
			if ( count > mCapacity )
				Allocate( count );

			pointer data_ptr = GetDataPointer();
			pointer end_ptr = data_ptr + count;

			// Move elements from previous memory
			for ( auto it_ptr = data_ptr + mSize; it_ptr != end_ptr; it_ptr++ )
			{
				std::construct_at( it_ptr, std::forward< Args >( args )... );
				mSize++;
			}
		}

		constexpr void DestroyData( size_type start, size_type end )
		{
			auto data_ptr = GetDataPointer();
			auto end_ptr = data_ptr + end;

			for ( auto it_ptr = data_ptr + start; it_ptr != end_ptr; it_ptr++ )
			{
				std::destroy_at( it_ptr );
				mSize--;
			}
		}

		constexpr bool IsEmptyState() const
		{
			return mCapacity == 0;
		}

		constexpr bool IsUsingSmallBuffer() const
		{
			return mCapacity > 0 and mCapacity <= SmallBufferSize;
		}
		constexpr bool IsUsingLargeBuffer() const
		{
			return mCapacity > SmallBufferSize;
		}

	private:
		union Box
		{
			std::monostate none{};
			array_type stack;
			pointer heap;
		};

		Box mData{};
		size_type mSize{};
		size_type mCapacity{};

		AllocTy mAlloc;
	};
} // namespace slc
#pragma once

#include <slc/Common/Base.h>

namespace slc {

	namespace detail {

		template < typename Element, typename Key >
		concept HasKey = requires( const Element& e ) {
			{ e.first } -> std::convertible_to< Key >;
		};

		template < typename Key >
		struct KeyCompare
		{
			const Key key;

			constexpr KeyCompare( const Key& k ) noexcept
				: key( k )
			{}

			template < typename Element >
				requires HasKey< Element, Key >
			constexpr bool operator<( const Element& rhs ) const noexcept
			{
				return key < rhs.first;
			}
		};

		template < typename Key, typename Value >
		class MapElement
		{
		public:
			using KeyType = Key;
			using MappedType = Value;
			using ValueType = std::pair< Key, Value >;
			using CompareType = KeyCompare< Key >;

			constexpr MapElement( const KeyType& key, const MappedType& value ) noexcept
				: mPair( key, value )
			{}

			constexpr bool operator<( const MapElement& rhs ) const noexcept
			{
				return mPair.first < rhs.mPair.first;
			}
			constexpr bool operator<( const CompareType& rhs ) const noexcept
			{
				return mPair.first < rhs.key;
			}


			constexpr const auto& operator*() const noexcept
			{
				return mPair;
			}
			constexpr const auto* operator->() const noexcept
			{
				return &mPair;
			}

			constexpr void swap( MapElement& rhs ) noexcept
			{
				std::swap( mPair, rhs.mPair );
			}

		private:
			ValueType mPair;
		};


		template < typename Key, typename Hasher = std::hash< Key > >
		class KeyCompareHash : public KeyCompare< Key >
		{
			using BaseType = KeyCompare< Key >;

		public:
			constexpr KeyCompareHash( const Key& key ) noexcept
				: BaseType( key ), mHash( Hasher()( key ) )
			{
			}

			template < typename Element >
			constexpr bool operator<( const Element& rhs ) const noexcept
			{
				return mHash < rhs.mHash || ( !( rhs.mHash < mHash ) && BaseType::operator<( rhs ) );
			}

		private:
			const std::size_t mHash;
		};


		template < typename Key, typename Value, typename Hasher = std::hash< Key > >
		class MapElementHash : public MapElement< Key, Value >
		{
			using BaseType = MapElement< Key, Value >;

		public:
			using KeyType = Key;
			using MappedType = Value;
			using CompareType = KeyCompareHash< KeyType, Hasher >;

			friend CompareType;

			constexpr MapElementHash( const KeyType& key, const MappedType& value ) noexcept
				: BaseType( key, value ), mHash( Hasher()( key ) )
			{
			}

			template < typename T >
			constexpr bool operator<( const T& rhs ) const noexcept
			{
				return mHash < rhs.mHash || ( !( rhs.mHash < mHash ) && BaseType::operator<( rhs ) );
			}

			constexpr void swap( MapElementHash& rhs ) noexcept
			{
				std::swap( mHash, rhs.mHash );
				BaseType::swap( rhs );
			}

		private:
			std::size_t mHash;
		};

		template < typename Element >
		class StaticMapIterator
		{
		public:
			constexpr StaticMapIterator( const Element* pos ) noexcept
				: mPos( pos )
			{
			}

			constexpr bool operator==( const StaticMapIterator& rhs ) const noexcept
			{
				return mPos == rhs.mPos;
			}
			constexpr bool operator!=( const StaticMapIterator& rhs ) const noexcept
			{
				return mPos != rhs.mPos;
			}

			constexpr StaticMapIterator& operator++() noexcept
			{
				++mPos;
				return *this;
			}

			constexpr StaticMapIterator& operator+=( std::size_t i ) noexcept
			{
				mPos += i;
				return *this;
			}

			constexpr StaticMapIterator operator+( std::size_t i ) const noexcept
			{
				return mPos + i;
			}

			constexpr StaticMapIterator& operator--() noexcept
			{
				--mPos;
				return *this;
			}

			constexpr StaticMapIterator& operator-=( std::size_t i ) noexcept
			{
				mPos -= i;
				return *this;
			}

			constexpr std::size_t operator-( const StaticMapIterator& rhs ) const noexcept
			{
				return mPos - rhs.mPos;
			}

			constexpr const auto& operator*() const noexcept
			{
				return **mPos;
			}
			constexpr const auto* operator->() const noexcept
			{
				return &**mPos;
			}

		private:
			const Element* mPos;
		};

		struct Less
		{
			template < typename A, typename B >
			constexpr bool operator()( const A& a, const B& b ) const noexcept
			{
				return a < b;
			}
		};

		struct GreaterEqual
		{
			template < typename A, typename B >
			constexpr bool operator()( const A& a, const B& b ) const noexcept
			{
				return !( b < a );
			}
		};

		template < typename Compare, typename Iterator, typename Key >
		constexpr auto Bound( Iterator left, Iterator right, const Key& key ) noexcept
		{
			std::size_t count = right - left;
			while ( count > 0 )
			{
				const std::size_t step = count / 2;
				right = left + step;

				if ( Compare()( *right, key ) )
				{
					left = ++right;
					count -= step + 1;
				}
				else
				{
					count = step;
				}
			}
			return left;
		}
	} // namespace detail


	template < typename Element, std::size_t N >
	class StaticMap
	{
	public:
		static_assert( N > 0, "Map is empty" );

		using KeyType = typename Element::KeyType;
		using MappedType = typename Element::MappedType;
		using ValueType = typename Element::ValueType;
		using CompareType = typename Element::CompareType;

		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using const_reference = const ValueType&;
		using const_pointer = const ValueType*;
		using const_iterator = detail::StaticMapIterator< Element >;

		template < typename T >
		constexpr StaticMap( std::array< T, N > data ) noexcept
			: StaticMap( data, std::make_index_sequence< N >() )
		{
		}

	private:
		// Internal constructor
		template < typename T, std::size_t... I >
		constexpr StaticMap( std::array< T, N > data, std::index_sequence< I... > ) noexcept
			: mData{ { data[ I ].first, data[ I ].second }... }
		{
			static_assert( sizeof...( I ) == N, "index_sequence requires identical length" );

			// Bubble sort - should be evaluated at compile time though
			for ( auto left = mData, right = mData + N - 1; mData < right; right = left, left = mData )
			{
				for ( auto it = mData; it < right; ++it )
				{
					if ( it[ 1 ] < it[ 0 ] )
					{
						it[ 0 ].swap( it[ 1 ] );
						left = it;
					}
				}
			}
		}

	public:
		constexpr const MappedType& at( const KeyType& key ) const noexcept
		{
			return find( key )->second;
		}

		constexpr std::size_t size() const noexcept
		{
			return N;
		}

		constexpr const_iterator begin() const noexcept
		{
			return mData;
		}
		constexpr const_iterator cbegin() const noexcept
		{
			return begin();
		}
		constexpr const_iterator end() const noexcept
		{
			return mData + N;
		}
		constexpr const_iterator cend() const noexcept
		{
			return end();
		}

		// Creates copy of static map with additional element.
		constexpr StaticMap< Element, N + 1 > emplace( const KeyType& key, const MappedType& value ) const
		{
			return emplace( key, value, std::make_index_sequence< N >() );
		}

		constexpr const_iterator find( const KeyType& key ) const noexcept
		{
			const CompareType compareKey{ key };
			auto it = detail::Bound< std::less >( begin(), end(), compareKey );

			if ( it != end() and std::greater_equal()( *it, compareKey ) )
			{
				return it;
			}
			else
			{
				return end();
			}
		}

		constexpr bool contains( const KeyType& key ) const noexcept
		{
			return find( key ) != end();
		}

		constexpr const_iterator lower_bound( const KeyType& key ) const noexcept
		{
			return detail::Bound< detail::Less >( begin(), end(), CompareType{ key } );
		}
		constexpr const_iterator upper_bound( const KeyType& key ) const noexcept
		{
			return detail::Bound< detail::GreaterEqual >( begin(), end(), CompareType{ key } );
		}

	private:
		template < std::size_t... I >
		constexpr StaticMap< Element, N + 1 > emplace( const KeyType& key, const MappedType& value, std::index_sequence< I... > ) const
		{
			std::array< ValueType, N + 1 > data = { ( *mData[ I ] )..., { key, value } };
			return StaticMap< Element, N + 1 >( data );
		}

	private:
		Element mData[ N ];
	};
} // namespace slc
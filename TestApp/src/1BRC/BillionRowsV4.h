#pragma once

#include "streamline.h"

#include "Hash.h"

#include <print>

using namespace slc;

namespace v4 {

	namespace detail {

		struct Hash
		{
			using is_transparent = void; // enables heterogeneous lookup
			size_t operator()( std::string_view s ) const noexcept
			{
				return xxh::xxhash3< 64 >( s.begin(), s.end() );
			}
		};

		struct Equal
		{
			using is_transparent = void;
			bool operator()( std::string_view a, std::string_view b ) const noexcept
			{
				return a == b;
			}
		};

		struct Arena
		{
			std::size_t size = 0;

			void* mem_block = nullptr;
			void* head = nullptr;

			Arena( std::size_t size )
				: size( size ), mem_block( ::operator new( size ) ), head( mem_block )
			{}

			~Arena()
			{
				::operator delete( mem_block );
			}
		};

		template < typename T >
		class LinearAllocator
		{
		public:
			using value_type = T;

			LinearAllocator() noexcept = default;
			LinearAllocator( Arena* arena )
				: mArena( arena )
			{
			}


			template < class U >
			LinearAllocator( const LinearAllocator< U >& other ) noexcept
				: mArena( other.mArena )
			{
			}

			bool operator==( const LinearAllocator& other ) const noexcept
			{
				return mArena == other.mArena;
			}
			bool operator!=( const LinearAllocator& other ) const noexcept
			{
				return !( *this == other );
			}

			T* allocate( size_t n )
			{
				auto bytes = n * sizeof( T );

				auto base = static_cast< Byte* >( mArena->mem_block );
				auto head = static_cast< Byte* >( mArena->head );

				if ( head + bytes > base + mArena->size )
					throw std::bad_alloc();

				T* result = static_cast< T* >( mArena->head );
				mArena->head = head + bytes;
				return result;
			}

			void deallocate( T* ptr, std::size_t )
			{
				// No-op
				int here = 0;
			}


			template < class U >
			struct rebind
			{
				using other = LinearAllocator< U >;
			};

		private:
			Arena* mArena;

			template < typename U >
			friend class LinearAllocator;
		};
	} // namespace detail

	struct Entry
	{
		std::size_t count{};
		std::int64_t sum{};
		std::int16_t min = Limits< std::int16_t >::Max;
		std::int16_t max = Limits< std::int16_t >::Min;
	};

	class BillionRows
	{
	private:
		static std::size_t constexpr ChunkSize = 32_MB;
		static std::size_t constexpr ExtraLookAhead = 100;
		static std::size_t constexpr MaxUniqueElements = 10'000;

		using KeyAllocator = detail::LinearAllocator< char >;
		using StringKey = std::basic_string< char, std::char_traits< char >, KeyAllocator >;
		using MapAllocator = detail::LinearAllocator< std::pair< const StringKey, Entry > >;
		using MapType = std::unordered_map< StringKey, Entry, detail::Hash, detail::Equal, MapAllocator >;

		struct ThreadData
		{
			detail::Arena key_arena{ MaxUniqueElements * 100 * 2 };
			KeyAllocator key_alloc{ &key_arena };

			detail::Arena map_arena{ MaxUniqueElements * sizeof( MapType::value_type ) };
			MapAllocator map_alloc{ &map_arena };

			MapType records{ map_alloc };

			std::future< void > result;

			ThreadData()
			{
				records.reserve( MaxUniqueElements );
			}
		};

	public:
		BillionRows( std::string_view path )
			: mFile( path )
		{}

		void Run()
		{
			auto file_size = mFile.TotalSize();
			auto chunk_size = std::min( file_size - ExtraLookAhead, ChunkSize );
			auto chunk_count = mFile.TotalSize() / chunk_size;

			mThreadData.resize( chunk_count );

			for ( auto&& [ i, data ] : std::views::enumerate( mThreadData ) )
				data.result = mThreadPool.Queue( &BillionRows::RunThread, this, i, chunk_size );

			MergeResults();
		}

		void Print()
		{
			std::println( "{{" );
			for ( auto&& [ name, data ] : mRecords )
			{
				std::println( "\t{}:{:.1f}/{:.1f}/{:.1f},", name, static_cast< float >( data.min ) / 10, static_cast< float >( data.sum ) / ( data.count * 10 ), static_cast< float >( data.max ) / 10 );
			}
			std::println( "}}" );
		}

	private:
		void RunThread( std::size_t i, std::size_t chunk_size )
		{
			auto& thread_data = mThreadData[ i ];
			auto& result = thread_data.records;

			auto buffer = mFile.Slice( i * chunk_size, chunk_size + ExtraLookAhead );
			auto chunk_view = buffer.AsStringView();

			std::size_t chunk_bytes_read = i > 0 ? chunk_view.find_first_of( '\n', 0 ) + 1 : 0;

			for ( std::size_t pos = chunk_view.find_first_of( '\n', chunk_bytes_read ); pos != std::string_view::npos and chunk_bytes_read < chunk_size; pos = chunk_view.find_first_of( '\n', chunk_bytes_read ) )
			{
				auto line_view = chunk_view.substr( chunk_bytes_read, pos - chunk_bytes_read );
				chunk_bytes_read = pos + 1;

				auto split = line_view.find_first_of( ';' );

				std::string_view name = line_view.substr( 0, split );
				std::string_view data = line_view.substr( split + 1 );

				std::int16_t value = ParseNumber( data );

				auto it = result.find( name );
				if ( it != result.end() )
				{
					// key exists, just update entry
					auto& entry = it->second;
					entry.min = std::min( value, entry.min );
					entry.max = std::max( value, entry.max );
					entry.sum += value;
					entry.count++;
				}
				else
				{
					// key does not exist, construct and insert
					auto key = StringKey( name, thread_data.key_alloc );
					auto& entry = result.emplace( std::move( key ), Entry{} ).first->second;
					entry.min = value;
					entry.max = value;
					entry.sum = value;
					entry.count = 1;
				}
			}
		}

		void MergeResults()
		{
			std::ranges::for_each( mThreadData, &std::future< void >::wait, &ThreadData::result );

			for ( auto& result : mThreadData )
			{
				for ( auto&& [ name, data ] : result.records )
				{
					auto& entry = mRecords[ name ];
					entry.min = std::min( data.min, entry.min );
					entry.max = std::min( data.max, entry.max );
					entry.sum += data.sum;
					entry.count += data.count;
				}
			}
		}

		std::int16_t ParseNumber( std::string_view s )
		{
			bool negative = s.front() == '-';

			std::int16_t value = 0;
			std::size_t start_index = negative ? 1 : 0;

			for ( auto i = start_index; i < s.size(); i++ )
			{
				if ( s[ i ] != '.' )
				{
					value = value * 10 + ( s[ i ] - '0' );
				}
			}

			return negative ? -value : value;
		}

	private:
		fs::SharedFile mFile;

		ThreadPool mThreadPool{};
		std::vector< ThreadData > mThreadData;

		std::unordered_map< std::string_view, Entry > mRecords;
	};
} // namespace v4
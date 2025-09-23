#pragma once

#include "streamline.h"

#include <print>

using namespace slc;

namespace v2 {

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

		using MapType = std::unordered_map< std::string, Entry >;
		using ResultFuture = std::future< MapType >;

	public:
		BillionRows( std::string_view path )
			: mFile( path )
		{}

		void Run()
		{
			auto file_size = mFile.TotalSize();
			auto chunk_size = std::min( file_size - ExtraLookAhead, ChunkSize );
			auto chunk_count = mFile.TotalSize() / chunk_size;

			auto result_futures = std::vector< ResultFuture >{};
			result_futures.reserve( chunk_count );

			for ( auto i = 0; i < chunk_count; i++ )
				result_futures.push_back( mThreadPool.Queue( &BillionRows::RunThread, this, i, chunk_size ) );

			auto results = result_futures | std::views::transform( &ResultFuture::get );
			MergeResults( results );
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
		MapType RunThread( std::size_t i, std::size_t chunk_size )
		{
			MapType result{};

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

				auto& entry = result[ std::string( name ) ];
				entry.min = std::min( value, entry.min );
				entry.max = std::max( value, entry.max );
				entry.sum += value;
				entry.count++;
			}

			return result;
		}

		void MergeResults( auto& results )
		{
			for ( auto&& result : results )
			{
				for ( auto&& [ name, data ] : result )
				{
					auto& entry = mRecords[ std::move( name ) ];
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

		MapType mRecords;
	};
} // namespace v2
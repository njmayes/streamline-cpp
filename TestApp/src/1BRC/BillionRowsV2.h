#pragma once

#include "streamline.h"

#include <print>

using namespace slc;

class BillionRowsV2
{
private:
	static std::size_t constexpr ChunkSize = 64_KB;

	struct Entry
	{
		int min = Limits< int >::Max;
		int max = Limits< int >::Min;
		std::size_t count{};
		int sum{};
	};

	struct ThreadResult
	{
		std::map< std::string, Entry > records;

		std::vector< char > extra_bytes_start{};
		std::vector< char > extra_bytes_end{};
	};
	using ResultFuture = std::future< ThreadResult >;

	using MapType = std::map< std::string, Entry >;

public:
	BillionRowsV2( std::string_view path )
		: mFile( path )
	{}

	void Run()
	{
		auto chunk_count = mFile.TotalSize() / ChunkSize;

		auto result_futures = std::vector< ResultFuture >{};
		result_futures.reserve( chunk_count );

		for ( auto i = 0; i < chunk_count; i++ )
			result_futures.push_back( mThreadPool.Queue( &BillionRowsV2::RunThread, this, i ) );

		auto results = result_futures | std::views::transform( &ResultFuture::get ) | std::ranges::to< std::vector >();
		MergeResults( results );
	}

	void Print()
	{
		std::println( "{{" );
		for ( auto&& [ name, data ] : mRecords | std::ranges::to< std::vector >() )
		{
			std::println( "\t{}:{:.1f}/{:.1f}/{:.1f},", name, static_cast< float >( data.min ) / 10, static_cast< float >( data.sum ) / ( data.count * 10 ), static_cast< float >( data.max ) );
		}
		std::println( "}}" );
	}

private:
	ThreadResult RunThread( std::size_t i )
	{
		ThreadResult result{};

		auto buffer = mFile.Slice( i * ChunkSize, ChunkSize );
		auto chunk_view = buffer.AsStringView();

		std::string name;
		name.reserve( 256 );

		std::size_t chunk_bytes_read{};

		if ( i > 0 )
		{
			result.extra_bytes_start = chunk_view.substr( 0, chunk_view.find_first_of( '\n', chunk_bytes_read ) ) | std::ranges::to< std::vector >();
		}

		for ( std::size_t pos = chunk_view.find_first_of( '\n', chunk_bytes_read ); pos != std::string_view::npos; pos = chunk_view.find_first_of( '\n', chunk_bytes_read ) )
		{
			auto line_view = chunk_view.substr( chunk_bytes_read, pos - chunk_bytes_read );
			chunk_bytes_read = pos + 1;

			auto split = line_view.find_first_of( ';' );

			name = line_view.substr( 0, split );
			std::string_view data = line_view.substr( split + 1 );

			int value = ParseNumber( data );

			auto& entry = result.records[ name ];
			entry.min = std::min( value, entry.min );
			entry.max = std::max( value, entry.max );
			entry.sum += value;
			entry.count++;
		}

		result.extra_bytes_end = chunk_view.substr( chunk_bytes_read ) | std::ranges::to< std::vector >();

		return result;
	}

	void MergeResults( std::vector< ThreadResult > const& results )
	{
		for ( auto const& result_pair : std::views::slide( results, 2 ) )
		{
			auto const& result_a = result_pair[ 0 ];
			auto const& result_b = result_pair[ 1 ];

			auto extra = result_a.extra_bytes_end;
		}
	}

	void ParseEntry( std::string_view line, MapType& records )
	{
		auto split = line.find_first_of( ';' );

		auto name = line.substr( 0, split );
		std::string_view data = line.substr( split + 1 );

		int value = ParseNumber( data );

		auto& entry = records[ std::string( name ) ];
		entry.min = std::min( value, entry.min );
		entry.max = std::max( value, entry.max );
		entry.sum += value;
		entry.count++;
	}

	int ParseNumber( std::string_view data )
	{
		int pow = 0;
		int direction = std::isdigit( data.front() ) ? 1 : -1;
		int result = 0;

		for ( auto character : data )
		{
			if ( std::isdigit( character ) )
			{
				result += std::pow( ( character - '0' ), pow++ );
			}
		}
		return result * direction;
	}

private:
	fs::SharedFile mFile;
	ThreadPool mThreadPool{ 1 };

	MapType mRecords;
};
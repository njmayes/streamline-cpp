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
		float min{};
		float max{};
		std::size_t count{};
		float sum{};
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
		auto chunk_count = mFile.Size() / ChunkSize;

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
			std::println( "\t{}:{:.1f}/{:.1f}/{:.1f},", name, data.min, data.sum / data.count, data.max );
		}
		std::println( "}}" );
	}

	ThreadResult RunThread( std::size_t i )
	{
		ThreadResult result{};

		auto buffer = mFile.Use( i * ChunkSize, ChunkSize );
		auto chunk_view = buffer.AsView();

		std::string name;
		name.reserve( 256 );

		std::size_t chunk_bytes_read{};

		for ( std::size_t pos = chunk_view.find_first_of( '\n', chunk_bytes_read ); pos != std::string_view::npos; pos = chunk_view.find_first_of( '\n', chunk_bytes_read ) )
		{
			auto line_view = chunk_view.substr( chunk_bytes_read, pos - chunk_bytes_read );
			chunk_bytes_read = pos + 1;

			auto split = line_view.find_first_of( ';' );

			name = line_view.substr( 0, split );
			std::string_view data = line_view.substr( split + 1 );

			float value{};
			std::from_chars( data.data(), data.data() + data.size(), value );

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
	}

private:
	ipc::SharedFile mFile;
	ThreadPool mThreadPool{ 1 };

	MapType mRecords;
};
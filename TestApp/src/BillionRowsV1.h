#pragma once

#include "streamline.h"

#include <print>

using namespace slc;

class BillionRowsV1
{
public:
	BillionRowsV1( fs::path const& path )
		: mFile( path )
	{}

	void Run()
	{
		std::vector< char > buffer( TotalSize );
		std::string name;
		name.reserve( 256 );

		std::size_t read_bytes{};

		while ( mFile.read( buffer.data() + read_bytes, TotalSize - read_bytes ) || mFile.gcount() )
		{
			read_bytes = 0;

			auto view = std::string_view{ buffer.data(), TotalSize };
			for ( std::size_t pos = view.find_first_of( '\n', read_bytes ); pos != std::string_view::npos; pos = view.find_first_of( '\n', read_bytes ) )
			{
				auto line = std::string_view{ buffer.data() + read_bytes, pos - read_bytes };
				read_bytes = pos + 1;

				auto split = line.find_first_of( ';' );

				name = line.substr( 0, split );
				std::string_view data = line.substr( split + 1 );

				float value{};
				auto result = std::from_chars( data.data(), data.data() + data.size(), value );

				auto& entry = mRecords[ name ];
				entry.min = std::min( value, entry.min );
				entry.max = std::max( value, entry.max );
				entry.sum += value;
				entry.count++;
			}

			std::memmove( buffer.data(), buffer.data() + read_bytes, TotalSize - read_bytes );
			read_bytes = TotalSize - read_bytes;
		}
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



private:
	static auto constexpr TotalSize = 4_MB;

	std::ifstream mFile;

	struct Entry
	{
		float min{};
		float max{};
		std::size_t count{};
		float sum{};
	};

	std::unordered_map< std::string, Entry > mRecords;
};
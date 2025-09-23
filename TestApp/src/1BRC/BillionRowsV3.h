#pragma once

#include "streamline.h"

#include <print>

using namespace slc;

namespace v3 {

	struct Entry
	{
		std::size_t count{};
		std::int64_t sum{};
		std::int16_t min{};
		std::int16_t max{};
	};

	class FlatMap
	{
	public:
		FlatMap()
		{
			mFilled.reserve( 10000 );
		}

		void Record( std::string_view station, std::int16_t data )
		{
			SLC_PROFILE_FUNCTION();

			std::uint16_t slot = Lookup( station );

			if ( mKeys[ slot ].empty() )
			{
				mFilled.push_back( slot );
				mKeys[ slot ] = station;
				mValues[ slot ] = Entry{ 1, data, data, data };
				return;
			}

			auto& value = mValues[ slot ];
			value.min = std::min( value.min, data );
			value.max = std::max( value.max, data );
			value.sum += data;
			value.count++;
		}

		void Merge( std::string&& station, Entry&& data )
		{
			SLC_PROFILE_FUNCTION();

			std::uint16_t slot = Lookup( station );

			if ( mKeys[ slot ].empty() )
			{
				mFilled.push_back( slot );
				mKeys[ slot ] = std::move( station );
				mValues[ slot ] = std::move( data );
				return;
			}

			auto& value = mValues[ slot ];
			value.min = std::min( value.min, data.min );
			value.max = std::max( value.max, data.max );
			value.sum += data.sum;
			value.count += data.count;
		}

		auto View()
		{
			SLC_PROFILE_FUNCTION();

			Sort();

			auto make_data_pair = [ this ]( std::size_t slot ) {
				return std::make_pair( std::move( mKeys[ slot ] ), mValues[ slot ] );
			};
			return mFilled | std::views::transform( make_data_pair );
		}

	private:
		std::uint16_t Lookup( std::string_view station )
		{
			std::uint16_t slot = std::hash< std::string_view >{}( station );

			while ( not mKeys[ slot ].empty() )
			{
				if ( mKeys[ slot ] == station )
					break;

				slot++;
			}

			return slot;
		}

		void Sort()
		{
			auto cmp = [ this ]( std::size_t left, std::size_t right ) {
				return mKeys[ left ] < mKeys[ right ];
			};
			std::ranges::sort( mFilled, cmp );
		}

	private:
		std::array< std::string, Limits< uint16_t >::Max + 1 > mKeys{};
		std::array< Entry, Limits< uint16_t >::Max + 1 > mValues{};
		std::vector< std::uint16_t > mFilled{};
	};

	class BillionRows
	{
	private:
		static std::size_t constexpr ChunkSize = 32_MB;
		static std::size_t constexpr ExtraLookAhead = 100;

		using MapType = FlatMap;
		using ResultFuture = std::future< Box< MapType > >;

	public:
		BillionRows( std::string_view path )
			: mFile( path )
			, mRecords{ MakeBox< MapType >() }
		{}

		void Run()
		{
			SLC_PROFILE_FUNCTION();

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
			SLC_PROFILE_FUNCTION();

			std::println( "{{" );
			for ( auto&& [ name, data ] : mRecords->View() )
			{
				std::println( "\t{}:{:.1f}/{:.1f}/{:.1f},", name, static_cast< float >( data.min ) / 10, static_cast< float >( data.sum ) / ( data.count * 10 ), static_cast< float >( data.max ) / 10 );
			}
			std::println( "}}" );
		}

	private:
		Box< MapType > RunThread( std::size_t i, std::size_t chunk_size )
		{
			SLC_PROFILE_FUNCTION();

			Box< MapType > result = MakeBox< MapType >();

			auto buffer = mFile.Slice( i * chunk_size, chunk_size + ExtraLookAhead );
			auto chunk_view = buffer.AsStringView();

			std::size_t chunk_bytes_read = i > 0 ? chunk_view.find_first_of( '\n', 0 ) + 1 : 0;

			std::string_view current_name{};
			{
				SLC_PROFILE_SCOPE( "Parse Chunk" );

				auto base_ptr = chunk_view.data() + chunk_bytes_read;
				auto mid_ptr = base_ptr;
				for ( auto char_ptr = base_ptr; char_ptr < chunk_view.data() + chunk_view.size() and chunk_bytes_read < chunk_size; char_ptr++ )
				{
					if ( *char_ptr == ';' )
					{
						current_name = { base_ptr, static_cast< std::size_t >( char_ptr - base_ptr ) };

						char_ptr++;
						mid_ptr = char_ptr;
					}
					else if ( *char_ptr == '\n' )
					{
						auto data = std::string_view{ mid_ptr, static_cast< std::size_t >( char_ptr - mid_ptr ) };
						result->Record( current_name, ParseNumber( data ) );

						chunk_bytes_read += ( char_ptr - base_ptr + 1 );

						char_ptr++;
						base_ptr = char_ptr;
					}
				}
			}

			return result;
		}

		void MergeResults( auto& results )
		{
			SLC_PROFILE_FUNCTION();

			for ( auto&& result : results )
			{
				for ( auto&& [ name, data ] : result->View() )
				{
					mRecords->Merge( std::move( name ), std::move( data ) );
				}
			}
		}

		std::int16_t ParseNumber( std::string_view s )
		{
			SLC_PROFILE_FUNCTION();

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
		ThreadPool mThreadPool{ 1 };

		Box< MapType > mRecords;
	};
} // namespace v3
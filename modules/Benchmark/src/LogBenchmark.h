#include <benchmark/benchmark.h>

#include "SL/Core/Logging/BenchmarkLogger.h"
#include "SL/Core/Logging/Logger.h"
#include "SL/Core/Logging/Targets/ILogTarget.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sl::bench {

	namespace {

		template < typename T, std::ranges::input_range Range >
		std::vector< T > ToVector( Range&& range )
		{
			std::vector< T > out;

			if constexpr ( std::ranges::sized_range< Range > )
				out.reserve( std::ranges::size( range ) );

			for ( auto&& value : range )
				out.emplace_back( value );

			return out;
		}

		constexpr std::size_t GetNewLineBytes()
		{
#ifdef SL_PLATFORM_WINDOWS
			return 2;
#else
			return 1;
#endif
		}

		struct MessageSet
		{
			std::vector< std::vector< char > > storage;
			std::vector< MessageEntry > entries;
			std::size_t total_bytes = 0;
			std::size_t kept_messages = 0;
			std::size_t kept_bytes = 0;
		};

		struct PayloadSet
		{
			std::vector< std::string > storage;
			std::vector< std::string_view > views;
			std::size_t total_bytes = 0;
			std::size_t kept_messages = 0;
			std::size_t kept_bytes = 0;
		};

		MessageSet MakeMessages( std::size_t count, std::size_t message_size, std::uint32_t keep_percent )
		{
			MessageSet result;
			result.storage.reserve( count );
			result.entries.reserve( count );

			for ( std::size_t i = 0; i < count; ++i )
			{
				auto& message = result.storage.emplace_back( message_size );

				for ( std::size_t j = 0; j < message_size; ++j )
					message[ j ] = static_cast< char >( 'A' + ( ( i + j ) % 26 ) );

				auto const keep = static_cast< std::uint32_t >( i % 100 ) < keep_percent;
				auto const level = keep ? LogLevel::Error : LogLevel::Trace;

				result.entries.emplace_back( MessageEntry{ .message = std::span< char >( message.data(), message.size() ), .length = message.size(), .level = level } );

				result.total_bytes += message.size();

				if ( keep )
				{
					result.kept_messages++;
					result.kept_bytes += message.size();
				}
			}

			return result;
		}

		PayloadSet MakePayloads( std::size_t count, std::size_t message_size, std::uint32_t keep_percent )
		{
			PayloadSet result;
			result.storage.reserve( count );
			result.views.reserve( count );

			for ( std::size_t i = 0; i < count; ++i )
			{
				auto& message = result.storage.emplace_back();
				message.resize( message_size );

				for ( std::size_t j = 0; j < message_size; ++j )
					message[ j ] = static_cast< char >( 'A' + ( ( i + j ) % 26 ) );

				result.views.emplace_back( message );
				result.total_bytes += message.size();

				if ( static_cast< std::uint32_t >( i % 100 ) < keep_percent )
				{
					result.kept_messages++;
					result.kept_bytes += message.size();
				}
			}

			return result;
		}

		class BenchmarkLogTargetBase : public ILogTarget
		{
		public:
			BenchmarkLogTargetBase()
				: ILogTarget{ LogLevel::Info }
			{
			}

			std::size_t GetBytesWritten() const
			{
				return mToWrite;
			}

		private:
			void DoWriteTarget() override
			{
				benchmark::DoNotOptimize( mBuffer.data() );
				benchmark::DoNotOptimize( mToWrite );
			}

			void DoPreFlush() override
			{}
			void DoFlush() override
			{}
		};

		class CurrentPopulateTarget final : public BenchmarkLogTargetBase
		{
		};

		class FilterToVectorPopulateTarget final : public BenchmarkLogTargetBase
		{
		private:
			void PopulateBuffer( std::span< MessageEntry > data ) override
			{
				auto filtered_data =
					ToVector< MessageEntry >(
						data | std::views::filter(
								   [ this ]( auto const& entry ) {
									   return ShouldWriteMessage( entry );
								   }
							   )
					);

				mToWrite = 0;

				for ( auto const& entry : filtered_data )
				{
					PopulateBufferSingleEntry( entry );
					PopulateBufferNewLine();
				}
			}
		};

		class FilterToPointerVectorPopulateTarget final : public BenchmarkLogTargetBase
		{
		private:
			void PopulateBuffer( std::span< MessageEntry > data ) override
			{
				std::vector< MessageEntry const* > filtered_data;
				filtered_data.reserve( data.size() );

				for ( auto const& entry : data )
				{
					if ( ShouldWriteMessage( entry ) )
						filtered_data.emplace_back( &entry );
				}

				mToWrite = 0;

				for ( auto const* entry : filtered_data )
				{
					PopulateBufferSingleEntry( *entry );
					PopulateBufferNewLine();
				}
			}
		};

		class TwoPassCountThenWritePopulateTarget final : public BenchmarkLogTargetBase
		{
		private:
			void PopulateBuffer( std::span< MessageEntry > data ) override
			{
				mToWrite = 0;

				std::size_t required_bytes = 0;
				for ( auto const& entry : data )
				{
					if ( ShouldWriteMessage( entry ) )
						required_bytes += entry.length + GetNewLineBytes();
				}

				if ( required_bytes > mBuffer.size() )
				{
					auto new_size = mBuffer.size();
					while ( new_size < required_bytes )
						new_size *= 2;
					mBuffer.resize( new_size );
				}

				for ( auto const& entry : data )
				{
					if ( ShouldWriteMessage( entry ) )
					{
						PopulateBufferSingleEntry( entry );
						PopulateBufferNewLine();
					}
				}
			}
		};

		class NullLogTarget final : public ILogTarget
		{
		public:
			explicit NullLogTarget( LogLevel level = LogLevel::Trace )
				: ILogTarget( level )
			{
				SetInitialBufferSize( 256_KB );
			}

			std::size_t GetBytesWritten() const
			{
				return mToWrite;
			}

		private:
			void DoWriteTarget() override
			{
				benchmark::DoNotOptimize( mBuffer.data() );
				benchmark::DoNotOptimize( mToWrite );
			}

			void DoPreFlush() override
			{}
			void DoFlush() override
			{}
		};

		void AddNullTargets( BenchmarkLogger& logger, std::size_t target_count, LogLevel level = LogLevel::Trace )
		{
			auto& targets = logger.BenchmarkTargets();
			targets.clear();
			targets.reserve( target_count );

			for ( std::size_t i = 0; i < target_count; ++i )
				targets.push_back( MakeBox< NullLogTarget >( level ) );
		}

		void PrepareLoggerQueue(
			BenchmarkLogger& logger,
			std::span< std::string_view const > payloads,
			std::size_t message_size,
			std::uint32_t keep_percent
		)
		{
			logger.BenchmarkResetState();

			auto& queue = logger.BenchmarkQueue();
			auto& arena = logger.BenchmarkArena();

			queue.reserve( payloads.size() );

			for ( std::size_t i = 0; i < payloads.size(); ++i )
			{
				auto const keep = static_cast< std::uint32_t >( i % 100 ) < keep_percent;
				auto const level = keep ? LogLevel::Error : LogLevel::Trace;

				auto buffer = arena.RequestBuffer( logger.MessageSizeLimit() );
				SL_ASSERT( buffer.has_value(), "Failed to allocate logger benchmark buffer" );

				auto const& payload = payloads[ i ];
				auto const copy_size = std::min( message_size, payload.size() );

				std::memcpy( buffer->data(), payload.data(), copy_size );

				queue.emplace_back( MessageEntry{ .message = *buffer, .length = copy_size, .level = level } );
			}
		}

		template < typename Target >
		void RunPopulateBenchmark( benchmark::State& state )
		{
			auto const message_count = static_cast< std::size_t >( state.range( 0 ) );
			auto const message_size = static_cast< std::size_t >( state.range( 1 ) );
			auto const keep_percent = static_cast< std::uint32_t >( state.range( 2 ) );

			auto messages = MakeMessages( message_count, message_size, keep_percent );

			Target target;
			target.SetInitialBufferSize( ( message_size + GetNewLineBytes() ) * message_count + 1024 );

			for ( auto _ : state )
			{
				target.WriteTarget( messages.entries );
				benchmark::DoNotOptimize( target.GetBytesWritten() );
				benchmark::ClobberMemory();
			}

			state.SetItemsProcessed( static_cast< std::int64_t >( state.iterations() ) * static_cast< std::int64_t >( message_count ) );
			state.SetBytesProcessed( static_cast< std::int64_t >( state.iterations() ) * static_cast< std::int64_t >( messages.total_bytes ) );

			state.counters[ "msg_size" ] = static_cast< double >( message_size );
			state.counters[ "keep_pct" ] = static_cast< double >( keep_percent );
			state.counters[ "kept_msgs" ] = static_cast< double >( messages.kept_messages );
			state.counters[ "kept_bytes" ] = static_cast< double >( messages.kept_bytes );
		}

		template < typename Target >
		void RegisterPopulateBenchmarks( char const* name )
		{
			benchmark::RegisterBenchmark( name, &RunPopulateBenchmark< Target > )
				->Args( { 64, 32, 0 } )
				->Args( { 64, 32, 10 } )
				->Args( { 64, 32, 50 } )
				->Args( { 64, 32, 100 } )
				->Args( { 256, 64, 10 } )
				->Args( { 256, 64, 50 } )
				->Args( { 256, 64, 100 } )
				->Args( { 1024, 64, 10 } )
				->Args( { 1024, 64, 50 } )
				->Args( { 1024, 64, 100 } )
				->Args( { 4096, 128, 10 } )
				->Args( { 4096, 128, 50 } )
				->Args( { 4096, 128, 100 } );
		}

		static void BM_LogMemoryArena_RequestBuffer( benchmark::State& state )
		{
			auto const request_size = static_cast< std::size_t >( state.range( 0 ) );
			auto const request_count = static_cast< std::size_t >( state.range( 1 ) );

			LogMemoryArena arena{ request_size * request_count };

			for ( auto _ : state )
			{
				for ( std::size_t i = 0; i < request_count; ++i )
				{
					auto buffer = arena.RequestBuffer( request_size );
					benchmark::DoNotOptimize( buffer );
				}

				state.PauseTiming();
				arena.ReleaseBuffers();
				state.ResumeTiming();
			}

			state.SetItemsProcessed( static_cast< std::int64_t >( state.iterations() ) * static_cast< std::int64_t >( request_count ) );
			state.SetBytesProcessed( static_cast< std::int64_t >( state.iterations() ) * static_cast< std::int64_t >( request_size * request_count ) );
		}

		static void BM_LogMemoryArena_RequestBufferAndRelease( benchmark::State& state )
		{
			auto const request_size = static_cast< std::size_t >( state.range( 0 ) );
			auto const request_count = static_cast< std::size_t >( state.range( 1 ) );

			LogMemoryArena arena{ request_size * request_count };

			for ( auto _ : state )
			{
				for ( std::size_t i = 0; i < request_count; ++i )
				{
					auto buffer = arena.RequestBuffer( request_size );
					benchmark::DoNotOptimize( buffer );
				}

				arena.ReleaseBuffers();
				benchmark::ClobberMemory();
			}

			state.SetItemsProcessed( static_cast< std::int64_t >( state.iterations() ) * static_cast< std::int64_t >( request_count ) );
			state.SetBytesProcessed( static_cast< std::int64_t >( state.iterations() ) * static_cast< std::int64_t >( request_size * request_count ) );
		}

		static void BM_LogMemoryArena_RequestBufferOverflow( benchmark::State& state )
		{
			auto const request_size = static_cast< std::size_t >( state.range( 0 ) );
			auto const request_count = static_cast< std::size_t >( state.range( 1 ) );

			LogMemoryArena arena{ request_size * request_count };

			for ( auto _ : state )
			{
				for ( std::size_t i = 0; i < request_count; ++i )
				{
					auto buffer = arena.RequestBuffer( request_size );
					benchmark::DoNotOptimize( buffer );
				}

				auto overflow = arena.RequestBuffer( request_size );
				benchmark::DoNotOptimize( overflow );
				benchmark::ClobberMemory();

				state.PauseTiming();
				arena.ReleaseBuffers();
				state.ResumeTiming();
			}

			state.SetItemsProcessed( static_cast< std::int64_t >( state.iterations() ) * static_cast< std::int64_t >( request_count + 1 ) );
			state.SetBytesProcessed( static_cast< std::int64_t >( state.iterations() ) * static_cast< std::int64_t >( request_size * request_count ) );
		}

		static void BM_Logger_Flush_NullTarget( benchmark::State& state )
		{
			auto const message_count = static_cast< std::size_t >( state.range( 0 ) );
			auto const message_size = static_cast< std::size_t >( state.range( 1 ) );
			auto const keep_percent = static_cast< std::uint32_t >( state.range( 2 ) );
			auto const target_count = static_cast< std::size_t >( state.range( 3 ) );

			auto payloads = MakePayloads( message_count, message_size, keep_percent );
			Logger raw_logger{ Logger::MessageSizeLimit, message_count + 16 };
			raw_logger.SetLogLevel( LogLevel::Info );
			auto logger = raw_logger.GetBenchmarkLogger();
			AddNullTargets( logger, target_count, LogLevel::Info );;

			std::vector< std::string_view > payload_views;
			payload_views.reserve( payloads.views.size() );
			for ( auto const& view : payloads.views )
				payload_views.emplace_back( view );

			for ( auto _ : state )
			{
				state.PauseTiming();
				PrepareLoggerQueue( logger, payload_views, message_size, keep_percent );
				state.ResumeTiming();

				logger.BenchmarkFlush();
				benchmark::ClobberMemory();
			}

			state.SetItemsProcessed( static_cast< std::int64_t >( state.iterations() ) * static_cast< std::int64_t >( message_count ) );
			state.SetBytesProcessed( static_cast< std::int64_t >( state.iterations() ) * static_cast< std::int64_t >( payloads.total_bytes ) );

			state.counters[ "msg_size" ] = static_cast< double >( message_size );
			state.counters[ "keep_pct" ] = static_cast< double >( keep_percent );
			state.counters[ "targets" ] = static_cast< double >( target_count );
			state.counters[ "kept_msgs" ] = static_cast< double >( payloads.kept_messages );
			state.counters[ "kept_bytes" ] = static_cast< double >( payloads.kept_bytes );
		}

		static void BM_Logger_Log_StringView_NullTarget( benchmark::State& state )
		{
			auto const message_count = static_cast< std::size_t >( state.range( 0 ) );
			auto const message_size = static_cast< std::size_t >( state.range( 1 ) );
			auto const keep_percent = static_cast< std::uint32_t >( state.range( 2 ) );
			auto const target_count = static_cast< std::size_t >( state.range( 3 ) );

			auto payloads = MakePayloads( message_count, message_size, keep_percent );
			Logger raw_logger{ Logger::MessageSizeLimit, message_count + 16 };
			raw_logger.SetLogLevel( LogLevel::Info );
			auto logger = raw_logger.GetBenchmarkLogger();
			AddNullTargets( logger, target_count, LogLevel::Info );

			for ( auto _ : state )
			{
				for ( std::size_t i = 0; i < message_count; ++i )
				{
					auto const keep = static_cast< std::uint32_t >( i % 100 ) < keep_percent;
					auto const level = keep ? LogLevel::Error : LogLevel::Trace;

					logger.logger.Log( level, payloads.views[ i ] );
				}

				logger.BenchmarkFlush();
				benchmark::ClobberMemory();
			}

			state.SetItemsProcessed( static_cast< std::int64_t >( state.iterations() ) * static_cast< std::int64_t >( message_count ) );
			state.SetBytesProcessed( static_cast< std::int64_t >( state.iterations() ) * static_cast< std::int64_t >( payloads.total_bytes ) );

			state.counters[ "msg_size" ] = static_cast< double >( message_size );
			state.counters[ "keep_pct" ] = static_cast< double >( keep_percent );
			state.counters[ "targets" ] = static_cast< double >( target_count );
			state.counters[ "kept_msgs" ] = static_cast< double >( payloads.kept_messages );
			state.counters[ "kept_bytes" ] = static_cast< double >( payloads.kept_bytes );
		}

		static void BM_Logger_Log_Format1_NullTarget( benchmark::State& state )
		{
			auto const message_count = static_cast< std::size_t >( state.range( 0 ) );
			auto const message_size = static_cast< std::size_t >( state.range( 1 ) );
			auto const keep_percent = static_cast< std::uint32_t >( state.range( 2 ) );
			auto const target_count = static_cast< std::size_t >( state.range( 3 ) );

			auto payloads = MakePayloads( message_count, message_size, keep_percent );
			Logger raw_logger{ Logger::MessageSizeLimit, message_count + 16 };
			raw_logger.SetLogLevel( LogLevel::Info );
			auto logger = raw_logger.GetBenchmarkLogger();
			AddNullTargets( logger, target_count, LogLevel::Info );

			for ( auto _ : state )
			{
				for ( std::size_t i = 0; i < message_count; ++i )
				{
					auto const keep = static_cast< std::uint32_t >( i % 100 ) < keep_percent;
					auto const level = keep ? LogLevel::Error : LogLevel::Trace;

					logger.logger.Log( level, "{} {}", payloads.views[ i ], i );
				}

				logger.BenchmarkFlush();
				benchmark::ClobberMemory();
			}

			state.SetItemsProcessed( static_cast< std::int64_t >( state.iterations() ) * static_cast< std::int64_t >( message_count ) );
			state.SetBytesProcessed( static_cast< std::int64_t >( state.iterations() ) * static_cast< std::int64_t >( payloads.total_bytes ) );

			state.counters[ "msg_size" ] = static_cast< double >( message_size );
			state.counters[ "keep_pct" ] = static_cast< double >( keep_percent );
			state.counters[ "targets" ] = static_cast< double >( target_count );
			state.counters[ "kept_msgs" ] = static_cast< double >( payloads.kept_messages );
			state.counters[ "kept_bytes" ] = static_cast< double >( payloads.kept_bytes );
		}

		struct LogBenchmarkRegistrar
		{
			LogBenchmarkRegistrar()
			{
				RegisterPopulateBenchmarks< CurrentPopulateTarget >( "Log/LogTarget/PopulateBuffer/CurrentProduction" );
				RegisterPopulateBenchmarks< FilterToVectorPopulateTarget >( "Log/LogTarget/PopulateBuffer/FilterToVector" );
				RegisterPopulateBenchmarks< FilterToPointerVectorPopulateTarget >( "Log/LogTarget/PopulateBuffer/FilterToPointerVector" );
				RegisterPopulateBenchmarks< TwoPassCountThenWritePopulateTarget >( "Log/LogTarget/PopulateBuffer/TwoPassCountThenWrite" );

				benchmark::RegisterBenchmark( "Log/LogMemoryArena/RequestBuffer", &BM_LogMemoryArena_RequestBuffer )
					->Args( { 64, 64 } )
					->Args( { 64, 1024 } )
					->Args( { 128, 1024 } )
					->Args( { 512, 1024 } )
					->Args( { 512, 4096 } );

				benchmark::RegisterBenchmark( "Log/LogMemoryArena/RequestBufferAndRelease", &BM_LogMemoryArena_RequestBufferAndRelease )
					->Args( { 64, 64 } )
					->Args( { 64, 1024 } )
					->Args( { 128, 1024 } )
					->Args( { 512, 1024 } )
					->Args( { 512, 4096 } );

				benchmark::RegisterBenchmark( "Log/LogMemoryArena/RequestBufferOverflow", &BM_LogMemoryArena_RequestBufferOverflow )
					->Args( { 64, 64 } )
					->Args( { 128, 1024 } )
					->Args( { 512, 4096 } );

				benchmark::RegisterBenchmark( "Log/Logger/Flush/NullTarget", &BM_Logger_Flush_NullTarget )
					->Args( { 64, 32, 10, 1 } )
					->Args( { 64, 32, 100, 1 } )
					->Args( { 256, 64, 50, 1 } )
					->Args( { 1024, 64, 100, 1 } )
					->Args( { 1024, 64, 100, 4 } )
					->Args( { 4096, 128, 50, 1 } )
					->Args( { 4096, 128, 100, 4 } );

				benchmark::RegisterBenchmark( "Log/Logger/Log/StringView/NullTarget", &BM_Logger_Log_StringView_NullTarget )
					->Args( { 64, 32, 0, 1 } )
					->Args( { 64, 32, 100, 1 } )
					->Args( { 256, 64, 10, 1 } )
					->Args( { 256, 64, 100, 1 } )
					->Args( { 1024, 64, 50, 1 } )
					->Args( { 1024, 64, 100, 4 } );

				benchmark::RegisterBenchmark( "Log/Logger/Log/Format1/NullTarget", &BM_Logger_Log_Format1_NullTarget )
					->Args( { 64, 32, 0, 1 } )
					->Args( { 64, 32, 100, 1 } )
					->Args( { 256, 64, 10, 1 } )
					->Args( { 256, 64, 100, 1 } )
					->Args( { 1024, 64, 50, 1 } )
					->Args( { 1024, 64, 100, 4 } );
			}
		};

		[[maybe_unused]] static LogBenchmarkRegistrar sRegistrar{};

	} // namespace

} // namespace sl::bench
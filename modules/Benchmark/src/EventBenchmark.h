#include <benchmark/benchmark.h>

#include <array>
#include <barrier>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "SL/Core/Events/Event.h"
#include "SL/Core/Events/EventRuntime.h"

namespace sl::bench {

	// ------------------------------------------------------------
	// Benchmark event types
	// ------------------------------------------------------------

	struct BenchEventA
	{
		std::uint32_t value0 = 0;
		std::uint32_t value1 = 0;
	};

	struct BenchEventB
	{
		std::uint64_t id = 0;
		std::uint32_t size = 0;
		bool flag = false;
	};

	struct BenchEventC
	{
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
	};

	struct BenchEventD
	{
		std::array< std::uint8_t, 24 > payload{};
	};

	template < typename TEventList >
	using TestingEvent = EventView< TEventList >;

	using BenchEventList = TypeList<
		BenchEventA,
		BenchEventB,
		BenchEventC,
		BenchEventD >;

	using BenchSubEventList = TypeList<
		BenchEventA,
		BenchEventC >;

	using BenchEvent = TestingEvent< BenchEventList >;
	using BenchSubEvent = TestingEvent< BenchSubEventList >;

	using BenchRuntimeST = EventRuntimeST< BenchEventList >;
	using BenchRuntimeMTUnordered = EventRuntimeMT< BenchEventList >;
	using BenchRuntimeMTOrdered = EventRuntimeMTOrdered< BenchEventList >;

	enum class MixKind : int
	{
		Uniform = 0,
		Skewed = 1,
	};

	// ------------------------------------------------------------
	// Shared event generation
	// ------------------------------------------------------------

	static constexpr std::size_t SelectEventKind( std::size_t index, MixKind mix )
	{
		if ( mix == MixKind::Uniform )
			return index & 3ULL;

		const std::size_t bucket = index % 100ULL;
		if ( bucket < 85ULL )
			return 0;
		if ( bucket < 93ULL )
			return 1;
		if ( bucket < 98ULL )
			return 2;
		return 3;
	}

	static BenchEventA MakeEventA( std::size_t index )
	{
		return BenchEventA{
			.value0 = static_cast< std::uint32_t >( index ),
			.value1 = static_cast< std::uint32_t >( index + 1 ),
		};
	}

	static BenchEventB MakeEventB( std::size_t index )
	{
		return BenchEventB{
			.id = static_cast< std::uint64_t >( index ),
			.size = static_cast< std::uint32_t >( 64 + ( index % 128 ) ),
			.flag = ( index & 1ULL ) != 0,
		};
	}

	static BenchEventC MakeEventC( std::size_t index )
	{
		return BenchEventC{
			.x = static_cast< float >( index ),
			.y = static_cast< float >( index + 1 ),
			.z = static_cast< float >( index + 2 ),
		};
	}

	static BenchEventD MakeEventD( std::size_t index )
	{
		BenchEventD event{};
		for ( std::size_t i = 0; i < event.payload.size(); ++i )
			event.payload[ i ] = static_cast< std::uint8_t >( ( index + i ) & 0xFF );
		return event;
	}

	// ------------------------------------------------------------
	// Dispatch sink
	// ------------------------------------------------------------

	class DispatchSink
	{
	public:
		bool OnEventA( BenchEventA& event ) noexcept
		{
			mAccum += event.value0;
			mAccum += event.value1;
			return false;
		}

		bool OnEventB( BenchEventB& event ) noexcept
		{
			mAccum += event.id;
			mAccum += event.size;
			mAccum += static_cast< std::uint64_t >( event.flag );
			return false;
		}

		bool OnEventC( BenchEventC& event ) noexcept
		{
			mAccum += static_cast< std::uint64_t >( event.x + event.y + event.z );
			return false;
		}

		bool OnEventD( BenchEventD& event ) noexcept
		{
			for ( auto byte : event.payload )
				mAccum += byte;
			return false;
		}

		bool OnSubEvent( BenchSubEvent& event ) noexcept
		{
			return event.Dispatch(
				BindDispatch( this, &DispatchSink::OnEventA ),
				BindDispatch( this, &DispatchSink::OnEventC )
			);
		}

		void Reset() noexcept
		{
			mAccum = 0;
		}

		[[nodiscard]] std::uint64_t Value() const noexcept
		{
			return mAccum;
		}

	private:
		std::uint64_t mAccum = 0;
	};

	// ------------------------------------------------------------
	// Runtime listeners
	// ------------------------------------------------------------

	template < typename TRuntime >
	class BenchRuntimeListener : public BasicEventListener< TRuntime >
	{
	public:
		using Base = BasicEventListener< TRuntime >;
		using Event = typename Base::Event;

		explicit BenchRuntimeListener( DispatchSink* sink )
			: mSink( sink )
		{
		}

		SL_LISTENING_EVENTS( BenchEventA, BenchEventB, BenchEventC, BenchEventD )

		bool OnEvent( Event& event ) override
		{
			return event.Dispatch(
				BindDispatch( mSink, &DispatchSink::OnEventA ),
				BindDispatch( mSink, &DispatchSink::OnEventB ),
				BindDispatch( mSink, &DispatchSink::OnEventC ),
				BindDispatch( mSink, &DispatchSink::OnEventD )
			);
		}

	private:
		DispatchSink* mSink = nullptr;
	};

	template < typename TRuntime >
	class BenchRuntimeSubListener : public BasicEventListener< TRuntime >
	{
	public:
		using Base = BasicEventListener< TRuntime >;
		using Event = typename Base::Event;

		explicit BenchRuntimeSubListener( DispatchSink* sink )
			: mSink( sink )
		{
		}

		SL_LISTENING_EVENTS( BenchEventA, BenchEventB, BenchEventC, BenchEventD )

		bool OnEvent( Event& event ) override
		{
			return event.Dispatch(
				BindDispatch( mSink, &DispatchSink::OnSubEvent )
			);
		}

	private:
		DispatchSink* mSink = nullptr;
	};

	// ------------------------------------------------------------
	// Runtime posting helper
	// ------------------------------------------------------------

	template < typename TRuntime >
	static void RuntimePostRange( TRuntime& runtime, std::size_t start_index, std::size_t count, MixKind mix )
	{
		for ( std::size_t i = 0; i < count; ++i )
		{
			const std::size_t index = start_index + i;

			switch ( SelectEventKind( index, mix ) )
			{
				case 0:
					runtime.template Post< BenchEventA >( MakeEventA( index ) );
					break;
				case 1:
					runtime.template Post< BenchEventB >( MakeEventB( index ) );
					break;
				case 2:
					runtime.template Post< BenchEventC >( MakeEventC( index ) );
					break;
				default:
					runtime.template Post< BenchEventD >( MakeEventD( index ) );
					break;
			}
		}
	}

	// ------------------------------------------------------------
	// Single-thread benchmark helpers
	// ------------------------------------------------------------

	template < typename TRuntime >
	static void RuntimePostOnly_ST_Impl( benchmark::State& state )
	{
		const auto count = static_cast< std::size_t >( state.range( 0 ) );
		const auto mix = static_cast< MixKind >( state.range( 1 ) );

		TRuntime runtime;
		DispatchSink sink;
		auto listener = runtime.template CreateListener< BenchRuntimeListener< TRuntime > >( &sink );

		for ( auto _ : state )
		{
			state.PauseTiming();
			runtime.Dispatch();
			sink.Reset();
			state.ResumeTiming();

			RuntimePostRange( runtime, 0, count, mix );
		}

		state.PauseTiming();
		runtime.Dispatch();
		sink.Reset();
		state.ResumeTiming();

		state.SetItemsProcessed( static_cast< int64_t >( state.iterations() ) * static_cast< int64_t >( count ) );
	}

	template < typename TRuntime, typename TListener >
	static void RuntimeDispatchOnly_ST_Impl( benchmark::State& state )
	{
		const auto count = static_cast< std::size_t >( state.range( 0 ) );
		const auto mix = static_cast< MixKind >( state.range( 1 ) );

		TRuntime runtime;
		DispatchSink sink;
		auto listener = runtime.template CreateListener< TListener >( &sink );

		for ( auto _ : state )
		{
			state.PauseTiming();
			runtime.Dispatch();
			sink.Reset();
			RuntimePostRange( runtime, 0, count, mix );
			state.ResumeTiming();

			runtime.Dispatch();
		}

		state.PauseTiming();
		runtime.Dispatch();
		sink.Reset();
		state.ResumeTiming();

		state.SetItemsProcessed( static_cast< int64_t >( state.iterations() ) * static_cast< int64_t >( count ) );
	}

	template < typename TRuntime, typename TListener >
	static void RuntimeEndToEnd_ST_Impl( benchmark::State& state )
	{
		const auto count = static_cast< std::size_t >( state.range( 0 ) );
		const auto mix = static_cast< MixKind >( state.range( 1 ) );

		TRuntime runtime;
		DispatchSink sink;
		auto listener = runtime.template CreateListener< TListener >( &sink );

		for ( auto _ : state )
		{
			state.PauseTiming();
			sink.Reset();
			state.ResumeTiming();

			RuntimePostRange( runtime, 0, count, mix );
			runtime.Dispatch();
		}

		benchmark::DoNotOptimize( sink.Value() );
		benchmark::ClobberMemory();

		state.SetItemsProcessed( static_cast< int64_t >( state.iterations() ) * static_cast< int64_t >( count ) );
	}

	// ------------------------------------------------------------
	// Multi-thread benchmark helpers
	// ------------------------------------------------------------

	template < typename TRuntime, typename TListener >
	struct RuntimeMtContext
	{
		explicit RuntimeMtContext( int thread_count )
			: phase_barrier( thread_count )
		{
			listener = runtime.template CreateListener< TListener >( &sink );
		}

		TRuntime runtime;
		DispatchSink sink;
		Ref< TListener > listener;
		std::barrier<> phase_barrier;
	};

	template < typename TRuntime, typename TListener >
	static std::shared_ptr< RuntimeMtContext< TRuntime, TListener > > GetRuntimeMtContext( int thread_count )
	{
		static std::mutex mutex;
		static std::unordered_map< int, std::weak_ptr< RuntimeMtContext< TRuntime, TListener > > > contexts;

		std::scoped_lock lock( mutex );

		auto shared = contexts[ thread_count ].lock();
		if ( !shared )
		{
			shared = std::make_shared< RuntimeMtContext< TRuntime, TListener > >( thread_count );
			contexts[ thread_count ] = shared;
		}

		return shared;
	}

	template < typename TRuntime >
	static void RuntimePostOnly_MT_Impl( benchmark::State& state )
	{
		const auto per_thread_count = static_cast< std::size_t >( state.range( 0 ) );
		const auto mix = static_cast< MixKind >( state.range( 1 ) );

		auto context = GetRuntimeMtContext< TRuntime, BenchRuntimeListener< TRuntime > >( state.threads() );

		for ( auto _ : state )
		{
			context->phase_barrier.arrive_and_wait();

			const auto start_index = per_thread_count * static_cast< std::size_t >( state.thread_index() );
			RuntimePostRange( context->runtime, start_index, per_thread_count, mix );

			context->phase_barrier.arrive_and_wait();

			state.PauseTiming();
			if ( state.thread_index() == 0 )
			{
				context->runtime.Dispatch();
				context->sink.Reset();
			}
			context->phase_barrier.arrive_and_wait();
			state.ResumeTiming();
		}

		const auto total_per_iteration =
			static_cast< int64_t >( per_thread_count ) *
			static_cast< int64_t >( state.threads() );

		state.SetItemsProcessed( static_cast< int64_t >( state.iterations() ) * total_per_iteration );
	}

	template < typename TRuntime, typename TListener >
	static void RuntimeDispatchOnly_MT_Impl( benchmark::State& state )
	{
		const auto per_thread_count = static_cast< std::size_t >( state.range( 0 ) );
		const auto mix = static_cast< MixKind >( state.range( 1 ) );

		auto context = GetRuntimeMtContext< TRuntime, TListener >( state.threads() );

		for ( auto _ : state )
		{
			state.PauseTiming();
			if ( state.thread_index() == 0 )
			{
				context->runtime.Dispatch();
				context->sink.Reset();
			}
			context->phase_barrier.arrive_and_wait();

			const auto start_index = per_thread_count * static_cast< std::size_t >( state.thread_index() );
			RuntimePostRange( context->runtime, start_index, per_thread_count, mix );
			context->phase_barrier.arrive_and_wait();
			state.ResumeTiming();

			if ( state.thread_index() == 0 )
				context->runtime.Dispatch();

			context->phase_barrier.arrive_and_wait();
		}

		state.PauseTiming();
		if ( state.thread_index() == 0 )
		{
			context->runtime.Dispatch();
			context->sink.Reset();
		}
		context->phase_barrier.arrive_and_wait();
		state.ResumeTiming();

		const auto total_per_iteration =
			static_cast< int64_t >( per_thread_count ) *
			static_cast< int64_t >( state.threads() );

		state.SetItemsProcessed( static_cast< int64_t >( state.iterations() ) * total_per_iteration );
	}

	template < typename TRuntime, typename TListener >
	static void RuntimeEndToEnd_MT_Impl( benchmark::State& state )
	{
		const auto per_thread_count = static_cast< std::size_t >( state.range( 0 ) );
		const auto mix = static_cast< MixKind >( state.range( 1 ) );

		auto context = GetRuntimeMtContext< TRuntime, TListener >( state.threads() );

		for ( auto _ : state )
		{
			context->phase_barrier.arrive_and_wait();

			const auto start_index = per_thread_count * static_cast< std::size_t >( state.thread_index() );
			RuntimePostRange( context->runtime, start_index, per_thread_count, mix );

			context->phase_barrier.arrive_and_wait();

			if ( state.thread_index() == 0 )
				context->runtime.Dispatch();

			context->phase_barrier.arrive_and_wait();

			state.PauseTiming();
			if ( state.thread_index() == 0 )
			{
				benchmark::DoNotOptimize( context->sink.Value() );
				context->sink.Reset();
			}
			context->phase_barrier.arrive_and_wait();
			state.ResumeTiming();
		}

		const auto total_per_iteration =
			static_cast< int64_t >( per_thread_count ) *
			static_cast< int64_t >( state.threads() );

		state.SetItemsProcessed( static_cast< int64_t >( state.iterations() ) * total_per_iteration );
	}

	// ------------------------------------------------------------
	// Concrete benchmark entry points
	// ------------------------------------------------------------

	[[maybe_unused]] static void BM_RuntimeST_PostOnly( benchmark::State& state )
	{
		RuntimePostOnly_ST_Impl< BenchRuntimeST >( state );
	}

	[[maybe_unused]] static void BM_RuntimeST_DispatchOnly( benchmark::State& state )
	{
		RuntimeDispatchOnly_ST_Impl< BenchRuntimeST, BenchRuntimeListener< BenchRuntimeST > >( state );
	}

	[[maybe_unused]] static void BM_RuntimeST_SubDispatchOnly( benchmark::State& state )
	{
		RuntimeDispatchOnly_ST_Impl< BenchRuntimeST, BenchRuntimeSubListener< BenchRuntimeST > >( state );
	}

	[[maybe_unused]] static void BM_RuntimeST_EndToEnd( benchmark::State& state )
	{
		RuntimeEndToEnd_ST_Impl< BenchRuntimeST, BenchRuntimeListener< BenchRuntimeST > >( state );
	}

	[[maybe_unused]] static void BM_RuntimeST_EndToEnd_SubDispatch( benchmark::State& state )
	{
		RuntimeEndToEnd_ST_Impl< BenchRuntimeST, BenchRuntimeSubListener< BenchRuntimeST > >( state );
	}

	[[maybe_unused]] static void BM_RuntimeMT_Unordered_PostOnly( benchmark::State& state )
	{
		RuntimePostOnly_MT_Impl< BenchRuntimeMTUnordered >( state );
	}

	[[maybe_unused]] static void BM_RuntimeMT_Ordered_PostOnly( benchmark::State& state )
	{
		RuntimePostOnly_MT_Impl< BenchRuntimeMTOrdered >( state );
	}

	[[maybe_unused]] static void BM_RuntimeMT_Unordered_DispatchOnly( benchmark::State& state )
	{
		RuntimeDispatchOnly_MT_Impl<
			BenchRuntimeMTUnordered,
			BenchRuntimeListener< BenchRuntimeMTUnordered > >( state );
	}

	[[maybe_unused]] static void BM_RuntimeMT_Unordered_SubDispatchOnly( benchmark::State& state )
	{
		RuntimeDispatchOnly_MT_Impl<
			BenchRuntimeMTUnordered,
			BenchRuntimeSubListener< BenchRuntimeMTUnordered > >( state );
	}

	[[maybe_unused]] static void BM_RuntimeMT_Ordered_DispatchOnly( benchmark::State& state )
	{
		RuntimeDispatchOnly_MT_Impl<
			BenchRuntimeMTOrdered,
			BenchRuntimeListener< BenchRuntimeMTOrdered > >( state );
	}

	[[maybe_unused]] static void BM_RuntimeMT_Ordered_SubDispatchOnly( benchmark::State& state )
	{
		RuntimeDispatchOnly_MT_Impl<
			BenchRuntimeMTOrdered,
			BenchRuntimeSubListener< BenchRuntimeMTOrdered > >( state );
	}

	[[maybe_unused]] static void BM_RuntimeMT_Unordered_EndToEnd( benchmark::State& state )
	{
		RuntimeEndToEnd_MT_Impl<
			BenchRuntimeMTUnordered,
			BenchRuntimeListener< BenchRuntimeMTUnordered > >( state );
	}

	[[maybe_unused]] static void BM_RuntimeMT_Unordered_EndToEnd_SubDispatch( benchmark::State& state )
	{
		RuntimeEndToEnd_MT_Impl<
			BenchRuntimeMTUnordered,
			BenchRuntimeSubListener< BenchRuntimeMTUnordered > >( state );
	}

	[[maybe_unused]] static void BM_RuntimeMT_Ordered_EndToEnd( benchmark::State& state )
	{
		RuntimeEndToEnd_MT_Impl<
			BenchRuntimeMTOrdered,
			BenchRuntimeListener< BenchRuntimeMTOrdered > >( state );
	}

	[[maybe_unused]] static void BM_RuntimeMT_Ordered_EndToEnd_SubDispatch( benchmark::State& state )
	{
		RuntimeEndToEnd_MT_Impl<
			BenchRuntimeMTOrdered,
			BenchRuntimeSubListener< BenchRuntimeMTOrdered > >( state );
	}

	// ------------------------------------------------------------
	// Registration
	// ------------------------------------------------------------

#define DEFINE_ST_BENCHMARK( name, func )                  \
	for ( int size = 256; size <= ( 1 << 16 ); size *= 2 ) \
	{                                                      \
		benchmark::RegisterBenchmark(                      \
			name,                                          \
			&func                                          \
		)                                                  \
			->Args( { size, std::to_underlying( mix ) } ); \
	}

#define DEFINE_MT_BENCHMARK( name, func )                      \
	for ( int threads = 1; threads <= 8; threads *= 2 )        \
	{                                                          \
		for ( int size = 256; size <= ( 1 << 16 ); size *= 2 ) \
		{                                                      \
			benchmark::RegisterBenchmark(                      \
				name,                                          \
				&func                                          \
			)                                                  \
				->Args( { size, std::to_underlying( mix ) } )  \
				->Threads( threads );                          \
		}                                                      \
	}

	struct EventBenchmarkRegistrar
	{
		EventBenchmarkRegistrar()
		{
			constexpr MixKind mixes[] = {
				MixKind::Uniform,
				MixKind::Skewed
			};

			// ---------------- ST ----------------

			for ( MixKind mix : mixes )
			{
				// DEFINE_ST_BENCHMARK( std::format( "Event/{}/RuntimeST/PostOnly", mix ), BM_RuntimeST_PostOnly );
				DEFINE_ST_BENCHMARK( std::format( "Event/{}/RuntimeST/DispatchOnly", mix ), BM_RuntimeST_DispatchOnly );
				DEFINE_ST_BENCHMARK( std::format( "Event/{}/RuntimeST/SubDispatchOnly", mix ), BM_RuntimeST_SubDispatchOnly );
				DEFINE_ST_BENCHMARK( std::format( "Event/{}/RuntimeST/EndToEnd", mix ), BM_RuntimeST_EndToEnd );
				DEFINE_ST_BENCHMARK( std::format( "Event/{}/RuntimeST/EndToEnd_SubDispatch", mix ), BM_RuntimeST_EndToEnd_SubDispatch );
			}

			// ---------------- MT ----------------

			for ( MixKind mix : mixes )
			{
				// DEFINE_MT_BENCHMARK( std::format( "Event/{}/RuntimeMT_Unordered/PostOnly", mix ), BM_RuntimeMT_Unordered_PostOnly );
				DEFINE_MT_BENCHMARK( std::format( "Event/{}/RuntimeMT_Unordered/DispatchOnly", mix ), BM_RuntimeMT_Unordered_DispatchOnly );
				DEFINE_MT_BENCHMARK( std::format( "Event/{}/RuntimeMT_Unordered/SubDispatchOnly", mix ), BM_RuntimeMT_Unordered_SubDispatchOnly );
				DEFINE_MT_BENCHMARK( std::format( "Event/{}/RuntimeMT_Unordered/EndToEnd", mix ), BM_RuntimeMT_Unordered_EndToEnd );
				DEFINE_MT_BENCHMARK( std::format( "Event/{}/RuntimeMT_Unordered/EndToEnd_SubDispatch", mix ), BM_RuntimeMT_Unordered_EndToEnd_SubDispatch );
			}
		}
	};

#undef DEFINE_ST_BENCHMARK
#undef DEFINE_MT_BENCHMARK

	[[maybe_unused]] static EventBenchmarkRegistrar sRegistrar{};

} // namespace sl::bench

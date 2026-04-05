#pragma once

#include "IEventListener.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <new>
#include <ranges>
#include <span>
#include <thread>
#include <utility>
#include <vector>

namespace sl {

	template < typename TEventList, EventRuntimeMode Mode, EventOrdering Ordering >
	struct EventRuntimeQueueTraits;

	struct EventRuntimeConfig
	{
		std::size_t initial_buffer_size = 256;
	};

	template <
		typename TEventList,
		EventRuntimeMode Mode = EventRuntimeMode::MultiThreaded,
		EventOrdering Ordering = EventOrdering::GlobalOrdered >
	class BasicEventRuntime
	{
		static_assert( IsTypeList< TEventList >, "TEventList must satisfy IsTypeList" );

	public:
		using EventList = TEventList;
		using EventType = EventView< EventList >;
		using EventRecord = EventType::Record;
		using Listener = BasicEventListener< BasicEventRuntime< EventList, Mode, Ordering > >;
		using QueueTraits = EventRuntimeQueueTraits< EventList, Mode, Ordering >;
		using QueueState = typename QueueTraits::State;

	public:
		BasicEventRuntime()
			: BasicEventRuntime( EventRuntimeConfig{} )
		{
		}

		explicit BasicEventRuntime( EventRuntimeConfig config )
			: mConfig( config )
		{
			QueueTraits::Initialize( *this );
		}

		~BasicEventRuntime()
		{
			if constexpr ( Mode == EventRuntimeMode::MultiThreaded )
			{
				if ( mQueues.generation )
					mQueues.generation->fetch_add( 1, std::memory_order_release );
			}
		}

		BasicEventRuntime( const BasicEventRuntime& ) = delete;
		BasicEventRuntime& operator=( const BasicEventRuntime& ) = delete;
		BasicEventRuntime( BasicEventRuntime&& ) = delete;
		BasicEventRuntime& operator=( BasicEventRuntime&& ) = delete;

		template < typename T, typename... Args >
			requires DerivedFromOnly< T, Listener > && std::constructible_from< T, Args... >
		Ref< T > CreateListener( Args&&... args )
		{
			auto listener = Ref< T >::Create( std::forward< Args >( args )... );
			listener->mRuntime = this;
			RegisterListener( listener.Data() );
			return listener;
		}

		template < typename TEvent, typename... TArgs >
			requires IsRuntimeEvent< EventList, TEvent > && std::constructible_from< TEvent, TArgs... >
		void Post( TArgs&&... args )
		{
			QueueTraits::template Post< BasicEventRuntime, TEvent >( *this, std::forward< TArgs >( args )... );
		}

		void Dispatch()
		{
			ApplyPendingListeners();
			QueueTraits::Dispatch( *this );
		}

		const EventRuntimeConfig& GetConfig() const
		{
			return mConfig;
		}

	private:
		void ApplyPendingListeners()
		{
			std::scoped_lock listener_lock( mListeners.lock );

			if ( !mListeners.new_listeners.empty() )
			{
				mListeners.listeners.insert(
					mListeners.listeners.end(),
					mListeners.new_listeners.begin(),
					mListeners.new_listeners.end()
				);
				mListeners.new_listeners.clear();
			}

			if ( !mListeners.old_listeners.empty() )
			{
				std::erase_if(
					mListeners.listeners,
					[ & ]( const Listener* listener ) {
						return std::ranges::contains( mListeners.old_listeners, listener );
					}
				);
				mListeners.old_listeners.clear();
			}
		}

		void RegisterListener( Listener* listener )
		{
			std::scoped_lock listener_lock( mListeners.lock );
			mListeners.new_listeners.push_back( listener );
		}

		void DeregisterListener( Listener* listener )
		{
			std::scoped_lock listener_lock( mListeners.lock );
			mListeners.old_listeners.push_back( listener );
		}

		void DispatchOne( EventRecord& record )
		{
			EventType event( record );

			for ( Listener* listener : mListeners.listeners )
			{
				if ( event.IsHandled() )
					break;

				if ( listener->Accept( event ) )
					listener->OnEvent( event );
			}
		}

		void DispatchSpan( std::span< EventRecord > buffer )
		{
			for ( EventRecord& record : buffer )
				DispatchOne( record );
		}

	private:
		struct ListenerState
		{
			std::mutex lock;
			std::vector< Listener* > listeners;
			std::vector< Listener* > new_listeners;
			std::vector< Listener* > old_listeners;
		};

		template < typename, EventRuntimeMode, EventOrdering >
		friend struct EventRuntimeQueueTraits;

		template < typename >
		friend class BasicEventListener;
		friend class Application;

	private:
		ListenerState mListeners{};
		QueueState mQueues{};
		EventRuntimeConfig mConfig{};
	};

	// ============================================================
	// ST / Unordered
	// ============================================================

	template < typename TEventList >
	struct EventRuntimeQueueTraits< TEventList, EventRuntimeMode::SingleThreaded, EventOrdering::ThreadOrdered >
	{
		using EventRecord = detail::EventRecord< TEventList >;

		struct State
		{
			std::vector< EventRecord > write_buffer;
			std::vector< EventRecord > dispatch_buffer;
		};

		template < typename Runtime >
		static void Initialize( Runtime& runtime )
		{
			const std::size_t n = runtime.GetConfig().initial_buffer_size;
			runtime.mQueues.write_buffer.reserve( n );
			runtime.mQueues.dispatch_buffer.reserve( n );
		}

		template < typename Runtime, typename TEvent, typename... TArgs >
		static void Post( Runtime& runtime, TArgs&&... args )
		{
			runtime.mQueues.write_buffer.emplace_back(
				std::in_place_type< TEvent >,
				std::forward< TArgs >( args )...
			);
		}

		template < typename Runtime >
		static void Dispatch( Runtime& runtime )
		{
			std::swap( runtime.mQueues.write_buffer, runtime.mQueues.dispatch_buffer );
			runtime.DispatchSpan( runtime.mQueues.dispatch_buffer );
			runtime.mQueues.dispatch_buffer.clear();
		}
	};

	// ============================================================
	// ST / Ordered
	// ============================================================

	template < typename TEventList >
	struct EventRuntimeQueueTraits< TEventList, EventRuntimeMode::SingleThreaded, EventOrdering::GlobalOrdered > 
	{
		using EventRecord = detail::EventRecord< TEventList >;

		struct State
		{
			std::vector< EventRecord > write_buffer;
			std::vector< EventRecord > dispatch_buffer;
			std::uint64_t next_sequence = 0;
		};

		template < typename Runtime >
		static void Initialize( Runtime& runtime )
		{
			const std::size_t n = runtime.GetConfig().initial_buffer_size;
			runtime.mQueues.write_buffer.reserve( n );
			runtime.mQueues.dispatch_buffer.reserve( n );
		}

		template < typename Runtime, typename TEvent, typename... TArgs >
		static void Post( Runtime& runtime, TArgs&&... args )
		{
			runtime.mQueues.write_buffer.emplace_back(
				std::in_place_type< TEvent >,
				detail::ordered_event,
				runtime.mQueues.next_sequence++,
				std::forward< TArgs >( args )...
			);
		}

		template < typename Runtime >
		static void Dispatch( Runtime& runtime )
		{
			std::swap( runtime.mQueues.write_buffer, runtime.mQueues.dispatch_buffer );
			runtime.DispatchSpan( runtime.mQueues.dispatch_buffer );
			runtime.mQueues.dispatch_buffer.clear();
		}
	};

	// ============================================================
	// MT / Unordered
	// ============================================================

	template < typename TEventList >
	struct EventRuntimeQueueTraits< TEventList, EventRuntimeMode::MultiThreaded, EventOrdering::ThreadOrdered >
	{
		using EventRecord = detail::EventRecord< TEventList >;

		struct ThreadQueueControl
		{
			std::atomic< int > write_index;
			std::atomic< bool > posting;

			ThreadQueueControl()
				: write_index( 0 )
				, posting( false )
			{
			}
		};

		struct ThreadQueue
		{
			std::thread::id owner_thread_id{};
			ThreadQueueControl control{};
			std::vector< EventRecord > buffers[ 2 ];

			explicit ThreadQueue( std::size_t initial_buffer_size )
			{
				buffers[ 0 ].reserve( initial_buffer_size );
				buffers[ 1 ].reserve( initial_buffer_size );
			}
		};

		struct TlsQueueCache
		{
			const void* runtime = nullptr;
			ThreadQueue* queue = nullptr;
			std::uint64_t generation = 0;
		};

		struct State
		{
			std::atomic< bool > dispatching{ false };
			std::mutex registry_lock;
			std::vector< std::unique_ptr< ThreadQueue > > queues;
			std::shared_ptr< std::atomic< std::uint64_t > > generation;

			State()
				: generation( std::make_shared< std::atomic< std::uint64_t > >( 1 ) )
			{
			}
		};

		struct PostingScope
		{
			ThreadQueue* queue = nullptr;
			bool armed = false;

			explicit PostingScope( ThreadQueue& thread_queue )
				: queue( &thread_queue )
			{
			}

			void Enter()
			{
				queue->control.posting.store( true, std::memory_order_release );
				armed = true;
			}

			void Leave()
			{
				if ( !armed )
					return;

				queue->control.posting.store( false, std::memory_order_release );
				queue->control.posting.notify_one();
				armed = false;
			}

			~PostingScope()
			{
				Leave();
			}
		};

		template < typename Runtime >
		static void Initialize( Runtime& )
		{
		}

		template < typename Runtime >
		static ThreadQueue& GetOrCreateThreadQueue( Runtime& runtime )
		{
			thread_local TlsQueueCache cache{};

			const std::uint64_t generation = runtime.mQueues.generation->load( std::memory_order_acquire );
			if ( cache.runtime == &runtime && cache.queue != nullptr && cache.generation == generation )
				return *cache.queue;

			const auto thread_id = std::this_thread::get_id();

			std::scoped_lock registry_lock( runtime.mQueues.registry_lock );

			for ( const auto& queue : runtime.mQueues.queues )
			{
				if ( queue->owner_thread_id == thread_id )
				{
					cache.runtime = &runtime;
					cache.queue = queue.get();
					cache.generation = generation;
					return *queue;
				}
			}

			auto queue = std::make_unique< ThreadQueue >( runtime.GetConfig().initial_buffer_size );
			queue->owner_thread_id = thread_id;

			ThreadQueue* queue_ptr = queue.get();
			runtime.mQueues.queues.push_back( std::move( queue ) );

			cache.runtime = &runtime;
			cache.queue = queue_ptr;
			cache.generation = generation;
			return *queue_ptr;
		}

		template < typename Runtime, typename TEvent, typename... TArgs >
		static void Post( Runtime& runtime, TArgs&&... args )
		{
			ThreadQueue& queue = GetOrCreateThreadQueue( runtime );
			PostingScope posting_scope( queue );

			for ( ;; )
			{
				while ( runtime.mQueues.dispatching.load( std::memory_order_acquire ) )
					runtime.mQueues.dispatching.wait( true, std::memory_order_relaxed );

				posting_scope.Enter();

				if ( !runtime.mQueues.dispatching.load( std::memory_order_acquire ) )
					break;

				posting_scope.Leave();
			}

			const int write_index = queue.control.write_index.load( std::memory_order_relaxed );

			queue.buffers[ write_index ].emplace_back(
				std::in_place_type< TEvent >,
				std::forward< TArgs >( args )...
			);
		}

		template < typename Runtime >
		static void Dispatch( Runtime& runtime )
		{
			runtime.mQueues.dispatching.store( true, std::memory_order_release );

			std::vector< ThreadQueue* > queues;
			struct FrozenQueue
			{
				ThreadQueue* queue = nullptr;
				int read_index = 0;
			};

			std::vector< FrozenQueue > frozen;

			{
				std::scoped_lock registry_lock( runtime.mQueues.registry_lock );

				queues.reserve( runtime.mQueues.queues.size() );
				for ( const auto& queue : runtime.mQueues.queues )
					queues.push_back( queue.get() );

				frozen.reserve( queues.size() );

				for ( ThreadQueue* queue : queues )
				{
					while ( queue->control.posting.load( std::memory_order_acquire ) )
						queue->control.posting.wait( true, std::memory_order_relaxed );

					const int old_write = queue->control.write_index.load( std::memory_order_relaxed );
					queue->control.write_index.store( 1 - old_write, std::memory_order_relaxed );
					frozen.push_back( FrozenQueue{ .queue = queue, .read_index = old_write } );
				}
			}

			runtime.mQueues.dispatching.store( false, std::memory_order_release );
			runtime.mQueues.dispatching.notify_all();

			for ( const FrozenQueue& frozen_queue : frozen )
			{
				auto& buffer = frozen_queue.queue->buffers[ frozen_queue.read_index ];
				runtime.DispatchSpan( buffer );
				buffer.clear();
			}
		}
	};

	// ============================================================
	// MT / Ordered
	// ============================================================

	template < typename TEventList >
	struct EventRuntimeQueueTraits< TEventList, EventRuntimeMode::MultiThreaded, EventOrdering::GlobalOrdered >
	{
		using EventRecord = detail::EventRecord< TEventList >;

		struct ThreadQueueControl
		{
			std::atomic< int > write_index;
			std::atomic< bool > posting;

			ThreadQueueControl()
				: write_index( 0 )
				, posting( false )
			{
			}
		};

		struct ThreadQueue
		{
			std::thread::id owner_thread_id{};
			ThreadQueueControl control{};
			std::vector< EventRecord > buffers[ 2 ];

			explicit ThreadQueue( std::size_t initial_buffer_size )
			{
				buffers[ 0 ].reserve( initial_buffer_size );
				buffers[ 1 ].reserve( initial_buffer_size );
			}
		};

		struct TlsQueueCache
		{
			const void* runtime = nullptr;
			ThreadQueue* queue = nullptr;
			std::uint64_t generation = 0;
		};

		struct State
		{
			std::atomic< bool > dispatching{ false };
			std::atomic< std::uint64_t > ordering{ 0 };
			std::mutex registry_lock;
			std::vector< std::unique_ptr< ThreadQueue > > queues;
			std::shared_ptr< std::atomic< std::uint64_t > > generation;

			State()
				: generation( std::make_shared< std::atomic< std::uint64_t > >( 1 ) )
			{
			}
		};

		struct PostingScope
		{
			ThreadQueue* queue = nullptr;
			bool armed = false;

			explicit PostingScope( ThreadQueue& thread_queue )
				: queue( &thread_queue )
			{
			}

			void Enter()
			{
				queue->control.posting.store( true, std::memory_order_release );
				armed = true;
			}

			void Leave()
			{
				if ( !armed )
					return;

				queue->control.posting.store( false, std::memory_order_release );
				queue->control.posting.notify_one();
				armed = false;
			}

			~PostingScope()
			{
				Leave();
			}
		};

		template < typename Runtime >
		static void Initialize( Runtime& )
		{
		}

		template < typename Runtime >
		static ThreadQueue& GetOrCreateThreadQueue( Runtime& runtime )
		{
			thread_local TlsQueueCache cache{};

			const std::uint64_t generation = runtime.mQueues.generation->load( std::memory_order_acquire );
			if ( cache.runtime == &runtime && cache.queue != nullptr && cache.generation == generation )
				return *cache.queue;

			const auto thread_id = std::this_thread::get_id();

			std::scoped_lock registry_lock( runtime.mQueues.registry_lock );

			for ( const auto& queue : runtime.mQueues.queues )
			{
				if ( queue->owner_thread_id == thread_id )
				{
					cache.runtime = &runtime;
					cache.queue = queue.get();
					cache.generation = generation;
					return *queue;
				}
			}

			auto queue = std::make_unique< ThreadQueue >( runtime.GetConfig().initial_buffer_size );
			queue->owner_thread_id = thread_id;

			ThreadQueue* queue_ptr = queue.get();
			runtime.mQueues.queues.push_back( std::move( queue ) );

			cache.runtime = &runtime;
			cache.queue = queue_ptr;
			cache.generation = generation;
			return *queue_ptr;
		}

		template < typename Runtime, typename TEvent, typename... TArgs >
		static void Post( Runtime& runtime, TArgs&&... args )
		{
			ThreadQueue& queue = GetOrCreateThreadQueue( runtime );
			PostingScope posting_scope( queue );

			for ( ;; )
			{
				while ( runtime.mQueues.dispatching.load( std::memory_order_acquire ) )
					runtime.mQueues.dispatching.wait( true, std::memory_order_relaxed );

				posting_scope.Enter();

				if ( !runtime.mQueues.dispatching.load( std::memory_order_acquire ) )
					break;

				posting_scope.Leave();
			}

			const int write_index = queue.control.write_index.load( std::memory_order_relaxed );
			const std::uint64_t sequence =
				runtime.mQueues.ordering.fetch_add( 1, std::memory_order_relaxed );

			queue.buffers[ write_index ].emplace_back(
				std::in_place_type< TEvent >,
				detail::ordered_event,
				sequence,
				std::forward< TArgs >( args )...
			);
		}

		template < typename Runtime >
		static void Dispatch( Runtime& runtime )
		{
			runtime.mQueues.dispatching.store( true, std::memory_order_release );

			std::vector< ThreadQueue* > queues;
			struct FrozenQueue
			{
				ThreadQueue* queue = nullptr;
				int read_index = 0;
			};

			std::vector< FrozenQueue > frozen;

			{
				std::scoped_lock registry_lock( runtime.mQueues.registry_lock );

				queues.reserve( runtime.mQueues.queues.size() );
				for ( const auto& queue : runtime.mQueues.queues )
					queues.push_back( queue.get() );

				frozen.reserve( queues.size() );

				for ( ThreadQueue* queue : queues )
				{
					while ( queue->control.posting.load( std::memory_order_acquire ) )
						queue->control.posting.wait( true, std::memory_order_relaxed );

					const int old_write = queue->control.write_index.load( std::memory_order_relaxed );
					queue->control.write_index.store( 1 - old_write, std::memory_order_relaxed );
					frozen.push_back( FrozenQueue{ .queue = queue, .read_index = old_write } );
				}
			}

			runtime.mQueues.dispatching.store( false, std::memory_order_release );
			runtime.mQueues.dispatching.notify_all();

			struct Cursor
			{
				std::size_t frozen_index = 0;
				std::size_t event_index = 0;
			};

			auto compare = [ & ]( const Cursor& lhs, const Cursor& rhs ) {
				const auto& lhs_buffer = frozen[ lhs.frozen_index ].queue->buffers[ frozen[ lhs.frozen_index ].read_index ];
				const auto& rhs_buffer = frozen[ rhs.frozen_index ].queue->buffers[ frozen[ rhs.frozen_index ].read_index ];
				return lhs_buffer[ lhs.event_index ].sequence > rhs_buffer[ rhs.event_index ].sequence;
			};

			std::vector< Cursor > heap;
			heap.reserve( frozen.size() );

			for ( std::size_t i = 0; i < frozen.size(); ++i )
			{
				auto& buffer = frozen[ i ].queue->buffers[ frozen[ i ].read_index ];
				if ( !buffer.empty() )
					heap.push_back( Cursor{ .frozen_index = i, .event_index = 0 } );
			}

			std::make_heap( heap.begin(), heap.end(), compare );

			while ( !heap.empty() )
			{
				std::pop_heap( heap.begin(), heap.end(), compare );
				Cursor cursor = heap.back();
				heap.pop_back();

				auto& buffer = frozen[ cursor.frozen_index ].queue->buffers[ frozen[ cursor.frozen_index ].read_index ];
				runtime.DispatchOne( buffer[ cursor.event_index ] );

				++cursor.event_index;
				if ( cursor.event_index < buffer.size() )
				{
					heap.push_back( cursor );
					std::push_heap( heap.begin(), heap.end(), compare );
				}
			}

			for ( const FrozenQueue& frozen_queue : frozen )
				frozen_queue.queue->buffers[ frozen_queue.read_index ].clear();
		}
	};

	template < typename TEventList >
	using EventRuntimeST = BasicEventRuntime<
		TEventList,
		EventRuntimeMode::SingleThreaded,
		EventOrdering::ThreadOrdered >;

	template < typename TEventList >
	using EventRuntimeMT = BasicEventRuntime<
		TEventList,
		EventRuntimeMode::MultiThreaded,
		EventOrdering::ThreadOrdered >;

	template < typename TEventList >
	using EventRuntimeMTOrdered = BasicEventRuntime<
		TEventList,
		EventRuntimeMode::MultiThreaded,
		EventOrdering::GlobalOrdered >;
} // namespace sl
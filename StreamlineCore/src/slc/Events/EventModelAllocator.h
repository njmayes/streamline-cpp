#pragma once

#include "Event.h"

#include "slc/Allocators/LinearAllocator.h"

namespace slc {

	/// <summary>
	/// Allocates and constructs event models for any given event type.
	/// </summary>
	class ModelAllocator
	{
	private:
		using TypeName = std::string_view;
		SCONSTEXPR size_t DefaultModelChunkSize = 4;

		struct ModelState
		{
			Box< IAllocator > allocator;
			std::vector< EventConcept* > overflow;
		};

		using InternalAllocatorElement = std::pair< TypeName, ModelState >;
		using InternalAllocatorMap = std::map< TypeName, ModelState >;

		template < typename T >
		static InternalAllocatorElement BuildEventAllocator()
		{
			return std::make_pair( TypeTraits< T >::Name, ModelState{ MakeBox< LinearAllocator< EventModel< T > > >( DefaultModelChunkSize ), {} } );
		}

		template < typename... Args >
		static std::array< InternalAllocatorElement, sizeof...( Args ) > BuildAllEventAllocators( TypeList< Args... > default_types )
		{
			return { { BuildEventAllocator< Args >()... } };
		}

		template < typename... Args >
		inline static InternalAllocatorMap ConstructAllocatorMap( TypeList< Args... > default_types )
		{
			InternalAllocatorMap result;
			for ( auto&& [ type, allocator ] : BuildAllEventAllocators( default_types ) )
			{
				result.emplace( type, std::move( allocator ) );
			}
			return result;
		}

	public:
		ModelAllocator()
		{
		}

		template < typename... Args >
		ModelAllocator( TypeList< Args... > default_types )
			: mModelAllocators( ConstructAllocatorMap( default_types ) )
		{
		}

		~ModelAllocator()
		{
			for ( auto& [ type, model ] : mModelAllocators )
			{
				for ( auto ptr : model.overflow )
				{
					delete ptr;
				}
			}
		}

		ModelAllocator( const ModelAllocator& ) = delete;
		ModelAllocator& operator=( const ModelAllocator& ) = delete;

		ModelAllocator( ModelAllocator&& ) = default;
		ModelAllocator& operator=( ModelAllocator&& ) = default;

		void swap( ModelAllocator& other ) noexcept
		{
			std::swap( mModelAllocators, other.mModelAllocators );
		}

	public:
		/// <summary>
		/// Allocates and constructs a new event model for the event type T and returns a reference to it.
		/// Memory will be allocated from a pool allocator unless there is no more space in it this frame,
		/// in which case the memory will be allocated using default new operator. When Flush is called any
		/// of these default allocated pointers will be deleted and the pool allocator will be enlarged for
		/// the next frame.
		/// </summary>
		template < IsEvent T, typename... Args >
			requires std::constructible_from< T, Args... >
		EventModel< T >& NewModel( Args&&... args )
		{
			using EventType = TypeTraits< T >;

			if ( !mModelAllocators.contains( EventType::Name ) )
				Register< T >();

			auto& model = mModelAllocators.at( EventType::Name );

			// If there is no more space in allocator, just use default allocator.
			// Pointer will be saved to be cleared up on flush at which point the pool
			// will be enlarged. Don't do it here so we don't invalidate Events in queue.
			if ( not model.allocator->CanAllocate( 1 ) )
			{
				EventModel< T >* overflow = new EventModel< T >( std::forward< Args >( args )... );
				model.overflow.push_back( overflow );
				return *overflow;
			}

			return ConstructModel< T >( model.allocator, std::forward< Args >( args )... );
		}

		/// <summary>
		/// Clean up any events allocated this frame, and enlarge any pool allocators that filled
		/// up this frame. Any default allocated pointers from this frame are also deleted here.
		/// </summary>
		void Flush()
		{
			for ( auto& [ type, model ] : mModelAllocators )
			{
				// The allocator completely filled up during this frame. Reallocate larger to compensate.
				if ( not model.overflow.empty() )
				{
					model.allocator->ForceReallocate();
				}

				for ( auto ptr : model.overflow )
				{
					delete ptr;
				}
			}
		}

	private:
		template < IsEvent T >
		void Register()
		{
			using EventType = TypeTraits< T >;
			mModelAllocators.try_emplace( EventType::Name, MakeBox< LinearAllocator< EventModel< T > > >( DefaultModelChunkSize ) );
		}

		template < IsEvent T, typename... Args >
			requires std::constructible_from< T, Args... >
		static EventModel< T >& ConstructModel( Box< IAllocator >& allocator, Args&&... args )
		{
			auto memory = static_cast< EventModel< T >* >( allocator->Alloc( sizeof( EventModel< T > ) ) );
			std::construct_at( memory, std::forward< Args >( args )... );
			return *memory;
		}

	private:
		InternalAllocatorMap mModelAllocators;
	};
} // namespace slc
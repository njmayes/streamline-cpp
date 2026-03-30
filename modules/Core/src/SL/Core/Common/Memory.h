#pragma once

#include "Detail/Memory.h"
#include "Reflection.h"

#include <unordered_set>
#include <memory>

namespace sl {

	// Unique Pointer

	template < typename T >
	using Box = std::unique_ptr< T >;

	template < typename T, typename... Args >
	inline static constexpr Box< T > MakeBox( Args&&... args )
	{
		return std::make_unique< T >( std::forward< Args >( args )... );
	}


	// Shared Pointer (Intrusive)

	template < typename T >
	concept RefCountable = DerivedFromOnly< T, detail::RefCountedBase >;

	class RefCounted : public virtual detail::RefCountedBase
	{
	protected:
		RefCounted() = default;
		virtual ~RefCounted() = default;

		template < RefCountable T >
		friend class Ref;
	};

	namespace detail {

		class RefTracker
		{
		public:
			static bool IsTracked( void const* data );

			static void AddToReferenceTracker( void const* data );
			static void RemoveFromReferenceTracker( void const* data );

		private:
			inline static std::unordered_set< void const* > sRefSet;
		};
	} // namespace detail

	template < RefCountable T >
	class Ref
	{
	public:
		Ref()
			: mData( nullptr )
		{}
		Ref( std::nullptr_t )
			: mData( nullptr )
		{}
		Ref( T* data )
			: mData( data )
		{
			IncRef();
		}

		template < typename Other >
		Ref( const Ref< Other >& other )
		{
			mData = static_cast< T* >( other.mData );
			IncRef();
		}

		template < typename Other >
		Ref( Ref< Other >&& other )
		{
			mData = static_cast< T* >( other.mData );
			other.mData = nullptr;
		}

		Ref( const Ref< T >& other )
			: mData( other.mData )
		{
			IncRef();
		}

		~Ref()
		{
			DecRef();
		}

		Ref& operator=( std::nullptr_t )
		{
			DecRef();
			mData = nullptr;
			return *this;
		}

		Ref& operator=( const Ref< T >& other )
		{
			other.IncRef();
			DecRef();

			mData = static_cast< T* >( other.mData );
			return *this;
		}

		template < typename Other >
		Ref& operator=( const Ref< Other >& other )
		{
			other.IncRef();
			DecRef();

			mData = static_cast< T* >( other.mData );
			return *this;
		}

		template < typename Other >
		Ref& operator=( Ref< Other >&& other )
		{
			DecRef();

			mData = other.mData;
			other.mData = nullptr;
			return *this;
		}

		operator bool() const
		{
			return mData != nullptr;
		}

		T* operator->()
		{
			return mData;
		}
		const T* operator->() const
		{
			return mData;
		}

		T& operator*()
		{
			return *mData;
		}
		const T& operator*() const
		{
			return *mData;
		}

		T* Data()
		{
			return mData;
		}
		const T* Data() const
		{
			return mData;
		}

		template < typename U >
		bool operator==( const Ref< U >& other ) const
		{
			return mData == other.mData;
		}
		bool operator==( std::nullptr_t ) const
		{
			return mData == nullptr;
		}

		void Reset()
		{
			DecRef();
			mData = nullptr;
		}

		template < typename Other >
		Ref< Other > To() const
		{
			return Ref< Other >( *this );
		}

		template < typename... Args >
		static Ref< T > Create( Args&&... args )
		{
			return Ref< T >( new T( std::forward< Args >( args )... ) );
		}

	private:
		void IncRef() const
		{
			if ( !mData )
				return;

			mData->IncRefCount();
			detail::RefTracker::AddToReferenceTracker( static_cast< const void* >( mData ) );
		}

		void DecRef()
		{
			if ( !mData )
				return;

			mData->DecRefCount();
			if ( mData->GetRefCount() == 0 )
			{
				delete mData;
				detail::RefTracker::RemoveFromReferenceTracker( static_cast< const void* >( mData ) );
				mData = nullptr;
			}
		}

	private:
		template < RefCountable Other >
		friend class Ref;

		T* mData;
	};


	// Weak Pointer

	template < RefCountable T >
	class WeakRef
	{
	public:
		WeakRef() = default;

		template < typename U = T >
			requires std::derived_from< U, T >
		WeakRef( Ref< U > ref )
		{
			mData = ref.Data();
		}

		template < typename U = T >
			requires std::derived_from< U, T >
		WeakRef( U* instance )
		{
			mData = instance;
		}

		T* operator->()
		{
			return mData;
		}
		const T* operator->() const
		{
			return mData;
		}

		T& operator*()
		{
			return *mData;
		}
		const T& operator*() const
		{
			return *mData;
		}

		bool Valid() const
		{
			return mData ? detail::RefTracker::IsTracked( mData ) : false;
		}
		operator bool() const
		{
			return Valid();
		}

		Ref< T > Lock() const
		{
			return Valid() ? Ref< T >( mData ) : nullptr;
		}

	private:
		T* mData = nullptr;
	};
} // namespace sl

namespace std {
	template < typename T >
	struct hash;

	template < typename T >
	struct hash< sl::Ref< T > >
	{
		std::size_t operator()( const sl::Ref< T >& ref ) const
		{
			return std::hash< const void* >()( ( const void* )ref.Data() );
		}
	};

} // namespace std
#include "SharedMutex.h"

#ifdef SLC_PLATFORM_WINDOWS
#include <windows.h>
#elif defined( SLC_PLATFORM_LINUX )
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#endif

namespace sl::ipc {

	struct MutexDescriptor
	{
		std::string_view name;
		bool owner{};
#ifdef SLC_PLATFORM_WINDOWS
		HANDLE handle;
#elif defined( SLC_PLATFORM_LINUX )
		sem_t* sem{};
#endif
	};

#ifdef SLC_PLATFORM_WINDOWS
	MutexDescriptor CreateSharedMutex( std::string_view name )
	{
		MutexDescriptor desc{};

		desc.handle = CreateMutexA( nullptr, FALSE, name.data() );

		if ( not desc.handle )
			return {};

		return desc;
	}

	MutexDescriptor MapSharedMutex( std::string_view name )
	{
		MutexDescriptor desc{};

		desc.handle = OpenMutexA( SYNCHRONIZE, FALSE, name.data() );

		if ( not desc.handle )
			return {};

		return desc;
	}

	void CloseSharedMutex( MutexDescriptor desc )
	{
		if ( desc.handle )
			CloseHandle( desc.handle );
	}

	void LockSharedMutex( MutexDescriptor desc )
	{
		DWORD dw = WaitForSingleObject( desc.handle, INFINITE );
		if ( dw != WAIT_OBJECT_0 )
			throw std::runtime_error( "Lock mutex failed" );
	}

	void UnlockSharedMutex( MutexDescriptor desc )
	{
		if ( !ReleaseMutex( desc.handle ) )
			throw std::runtime_error( "Unlock mutex failed" );
	}

	bool CheckIsValid( MutexDescriptor desc )
	{
		return desc.handle;
	}
#endif

#ifdef SLC_PLATFORM_LINUX
	MutexDescriptor CreateSharedMutex( std::string_view name )
	{
		MutexDescriptor desc{};

		int oflag = O_CREAT;
		auto sem = sem_open( name.data(), oflag, 0666, 1 );
		if ( sem == SEM_FAILED )
			return {};

		desc.sem = sem;
		desc.name = name;
		desc.owner = true;

		return desc;
	}

	MutexDescriptor MapSharedMutex( std::string_view name )
	{
		MutexDescriptor desc{};

		int oflag = 0;
		auto sem = sem_open( name.data(), oflag, 0666, 1 );
		if ( sem == SEM_FAILED )
			return {};

		desc.sem = sem;
		desc.name = name;

		return desc;
	}

	void CloseSharedMutex( MutexDescriptor desc )
	{
		if ( desc.sem )
			sem_close( desc.sem );
		if ( desc.owner and not desc.name.empty() )
			sem_unlink( desc.name.data() );
	}

	void LockSharedMutex( MutexDescriptor desc )
	{
		if ( sem_wait( desc.sem ) == -1 )
			throw std::runtime_error( "Lock mutex failed" );
	}

	void UnlockSharedMutex( MutexDescriptor desc )
	{
		if ( sem_post( desc.sem ) == -1 )
			throw std::runtime_error( "Unlock mutex failed" );
	}

	bool CheckIsValid( MutexDescriptor desc )
	{
		return desc.sem;
	}
#endif

	struct SharedMutex::Impl
	{
		MutexDescriptor desc;
	};


	SharedMutex::SharedMutex( std::string_view name )
		: mImpl{ MakeBox< Impl >() }
	{
		mImpl->desc = MapSharedMutex( name );
		if ( not IsValid() )
			mImpl->desc = CreateSharedMutex( name );
	}

	SharedMutex::SharedMutex( SharedMutex&& ) noexcept = default;
	SharedMutex& SharedMutex::operator=( SharedMutex&& ) noexcept = default;

	SharedMutex::~SharedMutex()
	{
		CloseSharedMutex( mImpl->desc );
	}

	void SharedMutex::Lock()
	{
		LockSharedMutex( mImpl->desc );
	}

	void SharedMutex::Unlock()
	{
		UnlockSharedMutex( mImpl->desc );
	}

	bool SharedMutex::IsValid() const
	{
		return CheckIsValid( mImpl->desc );
	}
} // namespace sl::ipc
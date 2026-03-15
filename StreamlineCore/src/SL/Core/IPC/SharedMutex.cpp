#include "SharedMutex.h"

#include <string>

#ifdef SL_PLATFORM_WINDOWS
#include <windows.h>
#elif defined( SL_PLATFORM_LINUX ) || defined( SL_PLATFORM_MACOS )
#include <cerrno>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#endif

namespace sl::ipc {

	struct MutexDescriptor
	{
		std::string name;
		bool owner{};

#ifdef SL_PLATFORM_WINDOWS
		HANDLE handle{};
#elif defined( SL_PLATFORM_LINUX ) || defined( SL_PLATFORM_MACOS )
		sem_t* sem{ SEM_FAILED };
#endif
	};

#if defined( SL_PLATFORM_LINUX ) || defined( SL_PLATFORM_MACOS )
	namespace {

		std::string NormaliseSharedMutexName( std::string_view name )
		{
			std::string out;
			out.reserve( name.size() + 1 );

			if ( name.empty() || name.front() != '/' )
				out.push_back( '/' );

			for ( char ch : name )
				out.push_back( ch == '/' ? '_' : ch );

			if ( out.size() == 1 )
				throw std::runtime_error( "Invalid shared mutex name." );

			return out;
		}

	} // namespace
#endif

#ifdef SL_PLATFORM_WINDOWS
	MutexDescriptor CreateSharedMutex( std::string_view name )
	{
		MutexDescriptor desc{};
		desc.name = std::string{ name };
		desc.handle = CreateMutexA( nullptr, FALSE, desc.name.c_str() );

		if ( not desc.handle )
			return {};

		desc.owner = GetLastError() != ERROR_ALREADY_EXISTS;
		return desc;
	}

	MutexDescriptor MapSharedMutex( std::string_view name )
	{
		MutexDescriptor desc{};
		desc.name = std::string{ name };
		desc.handle = OpenMutexA( SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE, desc.name.c_str() );

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
		return desc.handle != nullptr;
	}
#endif

#if defined( SL_PLATFORM_LINUX ) || defined( SL_PLATFORM_MACOS )
	MutexDescriptor CreateSharedMutex( std::string_view name )
	{
		MutexDescriptor desc{};
		desc.name = NormaliseSharedMutexName( name );

		auto sem = sem_open( desc.name.c_str(), O_CREAT | O_EXCL, 0666, 1 );
		if ( sem == SEM_FAILED )
			return {};

		desc.sem = sem;
		desc.owner = true;

		return desc;
	}

	MutexDescriptor MapSharedMutex( std::string_view name )
	{
		MutexDescriptor desc{};
		desc.name = NormaliseSharedMutexName( name );

		auto sem = sem_open( desc.name.c_str(), 0 );
		if ( sem == SEM_FAILED )
			return {};

		desc.sem = sem;
		desc.owner = false;

		return desc;
	}

	void CloseSharedMutex( MutexDescriptor desc )
	{
		if ( desc.sem != SEM_FAILED )
			sem_close( desc.sem );

		if ( desc.owner && !desc.name.empty() )
			sem_unlink( desc.name.c_str() );
	}

	void LockSharedMutex( MutexDescriptor desc )
	{
		for ( ;; )
		{
			if ( sem_wait( desc.sem ) == 0 )
				return;

			if ( errno != EINTR )
				throw std::runtime_error( "Lock mutex failed" );
		}
	}

	void UnlockSharedMutex( MutexDescriptor desc )
	{
		if ( sem_post( desc.sem ) == -1 )
			throw std::runtime_error( "Unlock mutex failed" );
	}

	bool CheckIsValid( MutexDescriptor desc )
	{
		return desc.sem != SEM_FAILED;
	}
#endif

#if !defined( SL_PLATFORM_WINDOWS ) && !defined( SL_PLATFORM_LINUX ) && !defined( SL_PLATFORM_MACOS )

	auto CreateSharedMutex( std::string_view name ) -> MutexDescriptor
	{
		throw std::runtime_error( "Shared memory is not supported on this platform." );
	}

	auto MapSharedMutex( std::string_view name ) -> MutexDescriptor
	{
		throw std::runtime_error( "Shared memory is not supported on this platform." );
	}

	void CloseSharedMutex( MutexDescriptor desc )
	{
		throw std::runtime_error( "Shared memory is not supported on this platform." );
	}

	void LockSharedMutex( MutexDescriptor desc )
	{
		throw std::runtime_error( "Shared memory is not supported on this platform." );
	}

	void UnlockSharedMutex( MutexDescriptor desc )
	{
		throw std::runtime_error( "Shared memory is not supported on this platform." );
	}

	bool CheckIsValid( MutexDescriptor desc )
	{
		return false;
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
		{
			mImpl->desc = CreateSharedMutex( name );
			if ( not IsValid() )
				mImpl->desc = MapSharedMutex( name );
		}
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
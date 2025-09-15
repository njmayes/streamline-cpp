#include "SharedMemory.h"

#ifdef SLC_PLATFORM_WINDOWS
#include <windows.h>
#elif defined( SLC_PLATFORM_LINUX )
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace slc::ipc {

#ifdef SLC_PLATFORM_WINDOWS
	using FileHandle = HANDLE;
#elif defined( SLC_PLATFORM_LINUX )
	using FileHandle = int;
#endif

	struct FileDescriptor
	{
		std::string_view name;
		bool owner{};
		FileHandle handle;

	};

#ifdef SLC_PLATFORM_WINDOWS
	std::pair< Buffer, FileDescriptor > CreateSharedBuffer( std::string_view name, std::size_t size )
	{
		FileDescriptor desc{};

		desc.handle = CreateFileMappingA( INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, ( DWORD )size, name.data() );
		desc.name = name;
		desc.owner = true;

		if ( not desc.handle )
			return {};

		auto addr = MapViewOfFile( desc.handle, FILE_MAP_ALL_ACCESS, 0, 0, size );
		if ( not addr )
			return {};

		Buffer data{ addr, size, false };
		return std::make_pair( std::move( data ), desc );
	}

	std::pair< Buffer, FileDescriptor > MapSharedBuffer( std::string_view name, std::size_t size )
	{
		FileDescriptor desc{};

		desc.handle = OpenFileMappingA( FILE_MAP_ALL_ACCESS, FALSE, name.data() );
		desc.name = name;
		desc.owner = false;

		if ( not desc.handle )
			return {};

		auto addr = MapViewOfFile( desc.handle, FILE_MAP_ALL_ACCESS, 0, 0, size );
		if ( not addr )
			return {};

		Buffer data{ addr, size, false };
		return std::make_pair( std::move( data ), desc );
	}

	void CloseSharedBuffer( Buffer buffer, FileDescriptor desc )
	{
		auto data = buffer.Data();
		if ( data )
			UnmapViewOfFile( data );

		if ( desc.handle )
			CloseHandle( desc.handle );
	}
#endif

#ifdef SLC_PLATFORM_LINUX
	std::pair< Buffer, FileDescriptor > CreateSharedBuffer( std::string_view name, std::size_t size )
	{
		FileDescriptor desc{};
		desc.handle = -1;

		int oflag = O_CREAT | O_RDWR;
		desc.handle = shm_open( name.data(), oflag, 0666 );
		desc.name = name;
		desc.owner = true;

		if ( not desc.handle )
			return {};

		if ( ftruncate( desc.handle, size ) == -1 )
			return {};

		auto addr = mmap( nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, desc.handle, 0 );

		if ( not addr )
			return {};

		Buffer data{ addr, size, false };
		return std::make_pair( std::move( data ), desc );
	}

	std::pair< Buffer, FileDescriptor > MapSharedBuffer( std::string_view name, std::size_t size )
	{
		FileDescriptor desc{};

		int oflag = O_CREAT | O_RDWR;
		desc.handle = shm_open( name.data(), oflag, 0666 );
		desc.name = name;
		desc.owner = false;

		if ( not desc.handle )
			return {};

		auto addr = mmap( nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, desc.handle, 0 );

		if ( not addr )
			return {};

		Buffer data{ addr, size, false };
		return std::make_pair( std::move( data ), desc );
	}

	void CloseSharedBuffer( Buffer buffer, FileDescriptor desc )
	{
		auto data = buffer.Data();
		if ( data )
			munmap( data, buffer.Size() );

		if ( desc.handle != -1 )
			close( desc.handle );

		if ( desc.owner and not desc.name.empty() )
			shm_unlink( desc.name.data() );
	}
#endif

	struct SharedMemory::Impl
	{
		Buffer data;
		FileDescriptor desc;
	};


	SharedMemory::SharedMemory( std::string_view name, std::size_t size, bool create )
		: mImpl{ MakeBox< Impl >() }
	{
		auto&& [ buffer, desc ] = create ? CreateSharedBuffer( name, size ) : MapSharedBuffer( name, size );

		mImpl->data = std::move( buffer );
		mImpl->desc = desc;
	}

	SharedMemory::~SharedMemory()
	{
		CloseSharedBuffer( mImpl->data, mImpl->desc );
	}

	SharedBuffer SharedMemory::Get() const
	{
		return SharedBuffer{ mImpl->desc.name, mImpl->data };
	}
} // namespace slc::ipc
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
	std::tuple< Buffer, Buffer, FileDescriptor > CreateSharedBuffer( std::string_view name, std::size_t size )
	{
		auto constexpr offset = sizeof( std::size_t );
		auto const true_size = offset + size;
		auto const lo_order_size = static_cast< DWORD >( true_size & 0xffffffff );
		auto const hi_order_size = 0; // static_cast< DWORD >( ( size_  >> 32 ) & 0xffffffff );

		FileDescriptor desc{};

		desc.handle = CreateFileMappingA( INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, hi_order_size, lo_order_size, name.data() );
		desc.name = name;
		desc.owner = true;

		if ( not desc.handle )
			return {};

		auto addr = MapViewOfFile( desc.handle, FILE_MAP_ALL_ACCESS, 0, 0, true_size );
		if ( not addr )
		{
			CloseHandle( desc.handle );
			return {};
		}

		std::memcpy( addr, &true_size, offset );

		Buffer true_data{ addr, true_size, false };
		Buffer user_data = true_data.View( offset );

		return std::make_tuple( std::move( true_data ), std::move( user_data ), desc );
	}

	std::tuple< Buffer, Buffer, FileDescriptor > MapSharedBuffer( std::string_view name )
	{
		FileDescriptor desc{};

		desc.handle = OpenFileMappingA( FILE_MAP_ALL_ACCESS, FALSE, name.data() );
		desc.name = name;
		desc.owner = false;

		if ( not desc.handle )
			return {};

		auto constexpr offset = sizeof( std::size_t );
		auto header_ptr = MapViewOfFile( desc.handle, FILE_MAP_ALL_ACCESS, 0, 0, offset );
		if ( not header_ptr )
		{
			CloseHandle( desc.handle );
			return {};
		}

		auto const true_size = *static_cast< std::size_t const* >( header_ptr );
		auto true_addr = MapViewOfFile( desc.handle, FILE_MAP_ALL_ACCESS, 0, 0, true_size );

		Buffer true_data{ true_addr, true_size, false };
		Buffer user_data = true_data.View( offset );

		return std::make_tuple( std::move( true_data ), std::move( user_data ), desc );
	}

	void CloseSharedBuffer( Buffer buffer, FileDescriptor desc )
	{
		auto data = buffer.Data();
		if ( data )
			UnmapViewOfFile( data );

		if ( desc.handle )
			CloseHandle( desc.handle );
	}

	void DeleteSharedBuffer( FileDescriptor desc )
	{
		throw std::runtime_error( "Cannot manually delete a shared memory instance on Windows. This will happen automatically when the last usage ends." );
	}
#endif

#ifdef SLC_PLATFORM_LINUX
	std::tuple< Buffer, Buffer, FileDescriptor > CreateSharedBuffer( std::string_view name, std::size_t size )
	{
		auto constexpr offset = sizeof( std::size_t );
		auto const true_size = offset + size;

		FileDescriptor desc{};
		desc.handle = -1;

		int oflag = O_CREAT | O_RDWR;
		desc.handle = shm_open( name.data(), oflag, 0666 );
		desc.name = name;
		desc.owner = true;

		if ( not desc.handle )
			return {};

		if ( ftruncate( desc.handle, true_size ) == -1 )
		{
			close( desc.handle );
			return {};
		}

		auto addr = mmap( nullptr, true_size, PROT_READ | PROT_WRITE, MAP_SHARED, desc.handle, 0 );
		if ( addr == MAP_FAILED )
		{
			close( desc.handle );
			return {};
		}

		std::memcpy( addr, &true_size, offset );

		Buffer true_data{ addr, size, false };
		Buffer user_data = true_data.View( offset );

		return std::make_tuple( std::move( true_data ), std::move( user_data ), desc );
	}

	std::tuple< Buffer, Buffer, FileDescriptor > MapSharedBuffer( std::string_view name )
	{
		auto constexpr offset = sizeof( std::size_t );

		FileDescriptor desc{};

		int oflag = O_CREAT | O_RDWR;
		desc.handle = shm_open( name.data(), oflag, 0666 );
		desc.name = name;
		desc.owner = false;

		if ( not desc.handle )
			return {};

		auto header_ptr = mmap( nullptr, offset, PROT_READ | PROT_WRITE, MAP_SHARED, desc.handle, 0 );
		if ( header_ptr == MAP_FAILED )
		{
			close( desc.handle );
			return;
		}

		std::size_t total_size = *static_cast< size_t* >( header_ptr );
		munmap( header_ptr, offset );

		auto addr = mmap( nullptr, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, desc.handle, 0 );
		if ( addr == MAP_FAILED )
		{
			close( desc.handle );
			return;
		}

		Buffer true_data{ addr, true_size, false };
		Buffer user_data = true_data.View( offset );

		return std::make_tuple( std::move( true_data ), std::move( user_data ), desc );
	}

	void CloseSharedBuffer( Buffer buffer, FileDescriptor desc )
	{
		auto data = buffer.Data();
		if ( data )
			munmap( data, buffer.Size() );

		if ( desc.handle != -1 )
			close( desc.handle );
	}

	void DeleteSharedBuffer( FileDescriptor desc )
	{
		if ( not desc.owner )
			throw std::runtime_error( "Only the original creator of this shared memory can delete it." );

		shm_unlink( desc.name.data() );
	}
#endif

	struct SharedMemory::Impl
	{
		Buffer true_data;
		Buffer user_data;
		FileDescriptor desc;
	};


	SharedMemory::SharedMemory( std::string_view name, std::size_t size )
		: mImpl{ MakeBox< Impl >() }
	{
		auto&& [ true_buffer, user_buffer, desc ] = CreateSharedBuffer( name, size );

		mImpl->true_data = std::move( true_buffer );
		mImpl->user_data = std::move( user_buffer );
		mImpl->desc = desc;
	}


	SharedMemory::SharedMemory( std::string_view name )
		: mImpl{ MakeBox< Impl >() }
	{
		auto&& [ true_buffer, user_buffer, desc ] = MapSharedBuffer( name );

		mImpl->true_data = std::move( true_buffer );
		mImpl->user_data = std::move( user_buffer );
		mImpl->desc = desc;
	}

	SharedMemory SharedMemory::Create( std::string_view name, std::size_t size )
	{
		return SharedMemory( name, size );
	}

	SharedMemory SharedMemory::Acquire( std::string_view name )
	{
		return SharedMemory( name );
	}

	void SharedMemory::Delete( SharedMemory const& memory )
	{
		return DeleteSharedBuffer( memory.mImpl->desc );
	}

	SharedMemory::~SharedMemory()
	{
		CloseSharedBuffer( mImpl->true_data, mImpl->desc );
	}

	SharedBuffer SharedMemory::Use() const
	{
		return SharedBuffer{ mImpl->desc.name, mImpl->user_data };
	}
} // namespace slc::ipc
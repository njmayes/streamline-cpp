#include "SharedMemory.h"

#include "SharedMutex.h"

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
		FileHandle handle;
	};

	struct BufferHeader
	{
		std::size_t size;
		std::size_t ref_count;
	};

#ifdef SLC_PLATFORM_WINDOWS
	std::tuple< BufferView, BufferView, FileDescriptor > CreateSharedBuffer( std::string_view name, std::size_t size )
	{
		auto constexpr offset = sizeof( BufferHeader );
		auto const true_size = offset + size;
		auto const lo_order_size = static_cast< DWORD >( true_size & 0xffffffff );
		auto const hi_order_size = static_cast< DWORD >( ( true_size >> 32 ) & 0xffffffff );

		FileDescriptor desc{};

		desc.handle = CreateFileMappingA( INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, hi_order_size, lo_order_size, name.data() );
		desc.name = name;

		if ( not desc.handle )
			return {};

		auto addr = MapViewOfFile( desc.handle, FILE_MAP_ALL_ACCESS, 0, 0, true_size );
		if ( not addr )
		{
			CloseHandle( desc.handle );
			return {};
		}

		BufferHeader* header_ptr = static_cast< BufferHeader* >( addr );
		header_ptr->size = true_size;
		header_ptr->ref_count = 1;

		BufferView true_data{ addr, true_size };
		BufferView user_data = true_data.View( offset );

		return std::make_tuple( std::move( true_data ), std::move( user_data ), desc );
	}

	std::tuple< BufferView, BufferView, FileDescriptor > MapSharedBuffer( std::string_view name )
	{
		FileDescriptor desc{};

		desc.handle = OpenFileMappingA( FILE_MAP_ALL_ACCESS, FALSE, name.data() );
		desc.name = name;

		if ( not desc.handle )
			return {};

		auto constexpr offset = sizeof( BufferHeader );
		auto addr = MapViewOfFile( desc.handle, FILE_MAP_ALL_ACCESS, 0, 0, offset );
		if ( not addr )
		{
			CloseHandle( desc.handle );
			return {};
		}

		BufferHeader* header_ptr = static_cast< BufferHeader* >( addr );
		auto true_addr = MapViewOfFile( desc.handle, FILE_MAP_ALL_ACCESS, 0, 0, header_ptr->size );

		SharedMutex mutex( desc.name );
		mutex.Lock();
		header_ptr->ref_count += 1;
		mutex.Unlock();

		BufferView true_data{ true_addr, header_ptr->size };
		BufferView user_data = true_data.View( offset );

		return std::make_tuple( std::move( true_data ), std::move( user_data ), desc );
	}

	void CloseSharedBuffer( BufferView buffer, FileDescriptor desc )
	{
		auto data = buffer.Data();
		if ( data )
		{
			auto header_ptr = buffer.As< BufferHeader >();

			SharedMutex mutex( desc.name );
			mutex.Lock();
			header_ptr->ref_count -= 1;
			mutex.Unlock();

			UnmapViewOfFile( data );
		}

		if ( desc.handle )
			CloseHandle( desc.handle );
	}
#endif

#ifdef SLC_PLATFORM_LINUX
	std::tuple< BufferView, BufferView, FileDescriptor > CreateSharedBuffer( std::string_view name, std::size_t size )
	{
		auto constexpr offset = sizeof( BufferHeader );
		auto const true_size = offset + size;

		FileDescriptor desc{};
		desc.handle = -1;

		int oflag = O_CREAT | O_RDWR;
		desc.handle = shm_open( name.data(), oflag, 0666 );
		desc.name = name;

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

		BufferHeader* header_ptr = static_cast< BufferHeader* >( addr );
		header_ptr->size = true_size;
		header_ptr->ref_count = 1;

		BufferView true_data{ addr, size };
		BufferView user_data = true_data.View( offset );

		return std::make_tuple( std::move( true_data ), std::move( user_data ), desc );
	}

	std::tuple< BufferView, BufferView, FileDescriptor > MapSharedBuffer( std::string_view name )
	{
		auto constexpr offset = sizeof( BufferHeader );

		FileDescriptor desc{};

		int oflag = O_CREAT | O_RDWR;
		desc.handle = shm_open( name.data(), oflag, 0666 );
		desc.name = name;

		if ( not desc.handle )
			return {};

		auto base_addr = mmap( nullptr, offset, PROT_READ | PROT_WRITE, MAP_SHARED, desc.handle, 0 );
		if ( base_addr == MAP_FAILED )
		{
			close( desc.handle );
			return {};
		}

		BufferHeader header = *static_cast< BufferHeader* >( base_addr );
		munmap( base_addr, offset );

		auto addr = mmap( nullptr, header.size, PROT_READ | PROT_WRITE, MAP_SHARED, desc.handle, 0 );
		if ( addr == MAP_FAILED )
		{
			close( desc.handle );
			return {};
		}

		BufferHeader* header_ptr = static_cast<BufferHeader*>(addr);

		SharedMutex mutex( desc.name );
		mutex.Lock();
		header_ptr->ref_count += 1;
		mutex.Unlock();

		BufferView true_data{ addr, header_ptr->size };
		BufferView user_data = true_data.View( offset );

		return std::make_tuple( std::move( true_data ), std::move( user_data ), desc );
	}

	void CloseSharedBuffer( BufferView buffer, FileDescriptor desc )
	{
		bool unlink = false;

		auto data = buffer.Data();
		if ( data )
		{
			auto header_ptr = buffer.As< BufferHeader >();

			SharedMutex mutex( desc.name );
			mutex.Lock();
			header_ptr->ref_count -= 1;
			unlink = header_ptr->ref_count == 0;
			mutex.Unlock();

			munmap( data, buffer.Size() );
		}

		if ( desc.handle != -1 )
			close( desc.handle );

		if ( unlink )
			shm_unlink( desc.name.data() );
	}
#endif

	struct SharedMemory::Impl
	{
		BufferView true_data;
		BufferView user_data;
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

	SharedMemory::~SharedMemory()
	{
		CloseSharedBuffer( mImpl->true_data, mImpl->desc );
	}

	SharedMemory SharedMemory::Create( std::string_view name, std::size_t size )
	{
		return SharedMemory( name, size );
	}

	SharedMemory SharedMemory::Acquire( std::string_view name )
	{
		return SharedMemory( name );
	}

	SharedBuffer SharedMemory::View()
	{
		return SharedBuffer{ mImpl->desc.name, mImpl->user_data };
	}

	BufferView const& SharedMemory::View() const
	{
		return mImpl->user_data;
	}
} // namespace slc::ipc
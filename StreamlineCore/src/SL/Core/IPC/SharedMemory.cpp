#include "SharedMemory.h"

#include "SharedMutex.h"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <string>
#include <thread>

#ifdef SL_PLATFORM_WINDOWS
#include <windows.h>
#elif defined( SL_PLATFORM_LINUX ) || defined( SL_PLATFORM_MACOS )
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace sl::ipc {

#ifdef SL_PLATFORM_WINDOWS
	using FileHandle = HANDLE;
#elif defined( SL_PLATFORM_LINUX ) || defined( SL_PLATFORM_MACOS )
	using FileHandle = int;
#endif

	struct FileDescriptor
	{
		std::string name;
		FileHandle handle{};
	};

	struct BufferHeader
	{
		std::size_t size{};
		std::size_t ref_count{};
		std::size_t user_size{};
		std::size_t ready{};
	};

#ifdef SL_PLATFORM_WINDOWS

	std::tuple< BufferView, BufferView, FileDescriptor > CreateSharedBuffer( std::string_view name, std::size_t size )
	{
		auto constexpr offset = sizeof( BufferHeader );
		auto const true_size = offset + size;
		auto const lo_order_size = static_cast< DWORD >( true_size & 0xffffffffull );
		auto const hi_order_size = static_cast< DWORD >( ( true_size >> 32 ) & 0xffffffffull );

		FileDescriptor desc{};
		desc.name = std::string{ name };
		desc.handle = CreateFileMappingA( INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, hi_order_size, lo_order_size, desc.name.c_str() );

		if ( not desc.handle )
			return {};

		auto addr = MapViewOfFile( desc.handle, FILE_MAP_ALL_ACCESS, 0, 0, true_size );
		if ( not addr )
		{
			CloseHandle( desc.handle );
			return {};
		}

		auto* header_ptr = static_cast< BufferHeader* >( addr );
		header_ptr->size = true_size;
		header_ptr->ref_count = 1;
		header_ptr->user_size = size;
		header_ptr->ready = 1;

		BufferView true_data{ addr, true_size };
		BufferView user_data = true_data.View( offset );

		return std::make_tuple( std::move( true_data ), std::move( user_data ), std::move( desc ) );
	}

	std::tuple< BufferView, BufferView, FileDescriptor > MapSharedBuffer( std::string_view name )
	{
		FileDescriptor desc{};
		desc.name = std::string{ name };
		desc.handle = OpenFileMappingA( FILE_MAP_ALL_ACCESS, FALSE, desc.name.c_str() );

		if ( not desc.handle )
			return {};

		auto constexpr offset = sizeof( BufferHeader );

		auto header_addr = MapViewOfFile( desc.handle, FILE_MAP_ALL_ACCESS, 0, 0, offset );
		if ( not header_addr )
		{
			CloseHandle( desc.handle );
			return {};
		}

		auto header = *static_cast< BufferHeader* >( header_addr );
		UnmapViewOfFile( header_addr );

		if ( header.ready != 1 || header.size < offset )
		{
			CloseHandle( desc.handle );
			return {};
		}

		auto addr = MapViewOfFile( desc.handle, FILE_MAP_ALL_ACCESS, 0, 0, header.size );
		if ( not addr )
		{
			CloseHandle( desc.handle );
			return {};
		}

		auto* header_ptr = static_cast< BufferHeader* >( addr );

		SharedMutex mutex( desc.name );
		mutex.Lock();
		header_ptr->ref_count += 1;
		mutex.Unlock();

		BufferView true_data{ addr, header_ptr->size };
		BufferView user_data = true_data.View( offset );

		return std::make_tuple( std::move( true_data ), std::move( user_data ), std::move( desc ) );
	}

	void CloseSharedBuffer( BufferView buffer, FileDescriptor desc )
	{
		auto data = buffer.Data();
		if ( data )
		{
			auto* header_ptr = buffer.As< BufferHeader >();

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

#if defined( SL_PLATFORM_LINUX ) || defined( SL_PLATFORM_MACOS )

	namespace {

		std::string NormaliseSharedObjectName( std::string_view name )
		{
			std::string out;
			out.reserve( name.size() + 1 );

			if ( name.empty() || name.front() != '/' )
				out.push_back( '/' );

			for ( char ch : name )
				out.push_back( ch == '/' ? '_' : ch );

			if ( out.size() == 1 )
				throw std::runtime_error( "Invalid shared memory name." );

			return out;
		}

		bool WaitForObjectToBeSized( int fd, std::size_t min_size )
		{
			for ( int i = 0; i < 5000; ++i )
			{
				struct stat st{};
				if ( fstat( fd, &st ) == -1 )
					return false;

				if ( static_cast< std::size_t >( st.st_size ) >= min_size )
					return true;

				std::this_thread::sleep_for( std::chrono::milliseconds{ 1 } );
			}

			return false;
		}

	} // namespace

	std::tuple< BufferView, BufferView, FileDescriptor > CreateSharedBuffer( std::string_view name, std::size_t size )
	{
		auto constexpr offset = sizeof( BufferHeader );
		auto const true_size = offset + size;

		FileDescriptor desc{};
		desc.name = NormaliseSharedObjectName( name );
		desc.handle = -1;

		desc.handle = shm_open( desc.name.c_str(), O_RDWR | O_CREAT | O_EXCL, 0666 );
		if ( desc.handle == -1 )
			return {};

		if ( ftruncate( desc.handle, static_cast< off_t >( true_size ) ) == -1 )
		{
			close( desc.handle );
			shm_unlink( desc.name.c_str() );
			return {};
		}

		auto addr = mmap( nullptr, true_size, PROT_READ | PROT_WRITE, MAP_SHARED, desc.handle, 0 );
		if ( addr == MAP_FAILED )
		{
			close( desc.handle );
			shm_unlink( desc.name.c_str() );
			return {};
		}

		auto* header_ptr = static_cast< BufferHeader* >( addr );
		header_ptr->size = true_size;
		header_ptr->ref_count = 1;
		header_ptr->user_size = size;
		header_ptr->ready = 1;

		BufferView true_data{ addr, true_size };
		BufferView user_data = true_data.View( offset );

		return std::make_tuple( std::move( true_data ), std::move( user_data ), std::move( desc ) );
	}

	std::tuple< BufferView, BufferView, FileDescriptor > MapSharedBuffer( std::string_view name )
	{
		auto constexpr offset = sizeof( BufferHeader );

		FileDescriptor desc{};
		desc.name = NormaliseSharedObjectName( name );
		desc.handle = shm_open( desc.name.c_str(), O_RDWR, 0666 );

		if ( desc.handle == -1 )
			return {};

		if ( not WaitForObjectToBeSized( desc.handle, offset ) )
		{
			close( desc.handle );
			return {};
		}

		auto header_addr = mmap( nullptr, offset, PROT_READ | PROT_WRITE, MAP_SHARED, desc.handle, 0 );
		if ( header_addr == MAP_FAILED )
		{
			close( desc.handle );
			return {};
		}

		auto header = *static_cast< BufferHeader* >( header_addr );
		munmap( header_addr, offset );

		if ( header.ready != 1 || header.size < offset )
		{
			close( desc.handle );
			return {};
		}

		auto addr = mmap( nullptr, header.size, PROT_READ | PROT_WRITE, MAP_SHARED, desc.handle, 0 );
		if ( addr == MAP_FAILED )
		{
			close( desc.handle );
			return {};
		}

		auto* header_ptr = static_cast< BufferHeader* >( addr );

		SharedMutex mutex( desc.name );
		mutex.Lock();
		if ( header_ptr->ready != 1 || header_ptr->size < offset )
		{
			mutex.Unlock();
			munmap( addr, header.size );
			close( desc.handle );
			return {};
		}
		header_ptr->ref_count += 1;
		mutex.Unlock();

		BufferView true_data{ addr, header_ptr->size };
		BufferView user_data = true_data.View( offset );

		return std::make_tuple( std::move( true_data ), std::move( user_data ), std::move( desc ) );
	}

	void CloseSharedBuffer( BufferView buffer, FileDescriptor desc )
	{
		bool unlink = false;

		auto data = buffer.Data();
		if ( data )
		{
			auto* header_ptr = buffer.As< BufferHeader >();

			SharedMutex mutex( desc.name );
			mutex.Lock();
			if ( header_ptr->ref_count > 0 )
			{
				header_ptr->ref_count -= 1;
				unlink = header_ptr->ref_count == 0;
			}
			mutex.Unlock();

			munmap( data, buffer.Size() );
		}

		if ( desc.handle != -1 )
			close( desc.handle );

		if ( unlink )
			shm_unlink( desc.name.c_str() );
	}

#endif

#if !defined( SL_PLATFORM_WINDOWS ) && !defined( SL_PLATFORM_LINUX ) && !defined( SL_PLATFORM_MACOS )

	std::tuple< BufferView, BufferView, FileDescriptor > CreateSharedBuffer( std::string_view name, std::size_t size )
	{
		throw std::runtime_error( "Shared memory is not supported on this platform." );
	}

	std::tuple< BufferView, BufferView, FileDescriptor > MapSharedBuffer( std::string_view name )
	{
		throw std::runtime_error( "Shared memory is not supported on this platform." );
	}

	void CloseSharedBuffer( BufferView buffer, FileDescriptor desc )
	{
		throw std::runtime_error( "Shared memory is not supported on this platform." );
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
		mImpl->desc = std::move( desc );
	}

	SharedMemory::SharedMemory( std::string_view name )
		: mImpl{ MakeBox< Impl >() }
	{
		auto&& [ true_buffer, user_buffer, desc ] = MapSharedBuffer( name );

		mImpl->true_data = std::move( true_buffer );
		mImpl->user_data = std::move( user_buffer );
		mImpl->desc = std::move( desc );
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

} // namespace sl::ipc
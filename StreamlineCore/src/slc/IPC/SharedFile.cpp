#include "SharedFile.h"

#include "SharedMutex.h"

#include "slc/Logging/Log.h"

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
	using Handle = HANDLE;
#elif defined( SLC_PLATFORM_LINUX )
	using Handle = int;
#endif

	struct FileDescriptor
	{
		std::string_view name;
		Handle file_handle;
		std::optional< Handle > map_handle{};
	};

#ifdef SLC_PLATFORM_WINDOWS
	void PrintError()
	{
		DWORD err = GetLastError();

		LPVOID msg;
		FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, ( LPSTR )&msg, 0, NULL );
		slc::log::Error( "{}", ( LPSTR )msg );
		LocalFree( msg );
	}


	FileDescriptor CreateSharedFile( std::string_view name )
	{
		std::wstring wname( name.begin(), name.end() );

		HANDLE hFile = CreateFileW(
			wname.c_str(),
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			nullptr,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			nullptr
		);
		if ( hFile == INVALID_HANDLE_VALUE )
			throw std::runtime_error( "CreateFileW failed" );

		HANDLE hMap = CreateFileMappingW(
			hFile,
			nullptr,
			PAGE_READWRITE,
			0,
			0,
			nullptr
		);
		if ( !hMap )
			throw std::runtime_error( "CreateFileMappingW failed" );

		// Return descriptor
		return FileDescriptor{ name, hFile, hMap };
	}

	SharedFileView MapSharedFile( FileDescriptor const& desc, std::size_t offset, std::size_t size )
	{
		SYSTEM_INFO si;
		GetSystemInfo( &si );
		DWORD granularity = si.dwAllocationGranularity;

		ULONGLONG alignedOffset = offset / granularity * granularity;
		DWORD offsetLow = static_cast< DWORD >( alignedOffset & 0xFFFFFFFF );
		DWORD offsetHigh = static_cast< DWORD >( ( alignedOffset >> 32 ) & 0xFFFFFFFF );

		size_t delta = static_cast< size_t >( offset - alignedOffset );

		void* base_ptr = MapViewOfFile(
			*desc.map_handle,
			FILE_MAP_READ | FILE_MAP_WRITE,
			offsetHigh,
			offsetLow,
			size + delta
		);

		if ( !base_ptr )
		{
			PrintError();
			throw std::runtime_error( "MapSharedViewOfFile failed" );
		}

		char* ptr = static_cast< char* >( base_ptr ) + delta;

		return SharedFileView{ static_cast< char* >( base_ptr ), ptr, size + delta, size };
	}

	void UnmapSharedRegion( char* base, std::size_t size )
	{
		UnmapViewOfFile( base );
	}

	void CloseSharedFile( FileDescriptor desc )
	{
		if ( desc.map_handle and desc.map_handle != INVALID_HANDLE_VALUE )
			CloseHandle( *desc.map_handle );

		if ( desc.file_handle != INVALID_HANDLE_VALUE )
			CloseHandle( desc.file_handle );
	}
#endif

#ifdef SLC_PLATFORM_LINUX
	FileDescriptor CreateSharedFile( std::string_view name )
	{
		int fd = open( name.data(), O_RDWR | O_CREAT, 0666 );
		if ( fd < 0 )
			throw std::runtime_error( "open failed" );

		return FileDescriptor{ name, fd };
	}

	SharedFileView MapSharedFile( FileDescriptor const& desc, std::size_t offset, std::size_t size )
	{
		size_t pagesize = sysconf( _SC_PAGE_SIZE );
		off_t aligned_offset = offset / pagesize * pagesize;
		size_t delta = offset - aligned_offset;

		void* base_ptr = mmap(
			nullptr,
			size + delta,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			desc.handle,
			static_cast< off_t >( aligned_offset )
		);

		if ( base_ptr == MAP_FAILED )
			throw std::runtime_error( "mmap failed" );

		char* ptr = static_cast< char* >( base_ptr ) + delta;

		return SharedFileView{ static_cast< char* >( base_ptr ), ptr, size + delta, size };
	}

	void UnmapSharedRegion( SharedFileView const& buffer )
	{
		munmap( buffer.Data(), buffer.Size() );
	}

	void CloseSharedFile( FileDescriptor desc )
	{
		if ( desc.handle >= 0 )
			close( desc.handle );
	}
#endif

	struct SharedFile::Impl
	{
		FileDescriptor desc;
	};

	SharedFile::SharedFile( std::string_view name )
		: mImpl{ MakeBox< Impl >() }
	{
		mImpl->desc = CreateSharedFile( name );
	}

	SharedFile::~SharedFile()
	{
		CloseSharedFile( mImpl->desc );
	}

	std::size_t SharedFile::Size() const
	{
		return fs::file_size( mImpl->desc.name );
	}

	SharedFileView SharedFile::Use( std::size_t offset, std::size_t size ) const
	{
		return MapSharedFile( mImpl->desc, offset, size );
	}

	SharedFileView::SharedFileView( char* base, char* data, std::size_t base_size, std::size_t data_size )
		: mBasePtr( base )
		, mDataPtr( data )
		, mBaseSize( base_size )
		, mDataSize( data_size )
	{
	}

	SharedFileView::SharedFileView( SharedFileView&& other ) noexcept
		: mBasePtr{ std::exchange( other.mBasePtr, nullptr ) }
		, mDataPtr{ std::exchange( other.mDataPtr, nullptr ) }
		, mBaseSize{ std::exchange( other.mBaseSize, 0 ) }
		, mDataSize{ std::exchange( other.mDataSize, 0 ) }
	{
	}

	SharedFileView& SharedFileView::operator=( SharedFileView&& other ) noexcept
	{
		mBasePtr = std::exchange( other.mBasePtr, nullptr );
		mDataPtr = std::exchange( other.mDataPtr, nullptr );
		mBaseSize = std::exchange( other.mBaseSize, 0 );
		mDataSize = std::exchange( other.mDataSize, 0 );

		return *this;
	}

	SharedFileView::~SharedFileView()
	{
		UnmapSharedRegion( mBasePtr, mBaseSize );
	}

	std::string_view SharedFileView::AsView() const
	{
		return std::string_view{ mDataPtr, mDataSize };
	}
} // namespace slc::ipc
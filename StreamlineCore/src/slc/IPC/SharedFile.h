#pragma once

#include "slc/Types/Buffer.h"

namespace slc::ipc {

	class SharedFileView
	{
	public:
		SharedFileView( char* base, char* data, std::size_t base_size, std::size_t data_size );

		SharedFileView( SharedFileView const&) = delete;
		SharedFileView( SharedFileView&& ) noexcept;

		SharedFileView& operator=( SharedFileView const& ) = delete;
		SharedFileView& operator=( SharedFileView&& ) noexcept;

		~SharedFileView();

		std::string_view AsView() const;

	private:
		char* mBasePtr;
		char* mDataPtr;

		std::size_t mBaseSize;
		std::size_t mDataSize;
	};

	class SharedFile
	{
	public:
		SharedFile( std::string_view path );

		SharedFile( SharedFile const& ) = delete;
		SharedFile( SharedFile&& ) = default;

		SharedFile& operator=( SharedFile const& ) = delete;
		SharedFile& operator=( SharedFile&& ) = default;

		virtual ~SharedFile();

		std::size_t Size() const;
		SharedFileView Use( std::size_t offset, std::size_t size ) const;

	private:
		struct Impl;
		Box< Impl > mImpl;
	};
} // namespace slc::ipc
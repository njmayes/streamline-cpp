#pragma once

#include "slc/Common/Base.h"

namespace slc::fs {

	class SharedFile
	{
	public:
		class Region
		{
		public:
			Region() = default;
			Region( Byte* base, Byte* data, std::size_t base_size, std::size_t data_size );

			Region( Region const& ) = delete;
			Region( Region&& ) noexcept;

			Region& operator=( Region const& ) = delete;
			Region& operator=( Region&& ) noexcept;

			~Region();

			std::string_view AsStringView() const;

			std::span< Byte > AsSpan();
			std::span< const Byte > AsSpan() const;

		private:
			Byte* mBasePtr{};
			Byte* mDataPtr{};

			std::size_t mBaseSize{};
			std::size_t mDataSize{};
		};

	public:
		SharedFile( std::string_view path );

		SharedFile( SharedFile const& ) = delete;
		SharedFile( SharedFile&& ) = default;

		SharedFile& operator=( SharedFile const& ) = delete;
		SharedFile& operator=( SharedFile&& ) = default;

		virtual ~SharedFile();

		std::size_t TotalSize() const;
		Region Slice( std::size_t offset, std::size_t size ) const;

	private:
		struct Impl;
		Box< Impl > mImpl;
	};
} // namespace slc::ipc
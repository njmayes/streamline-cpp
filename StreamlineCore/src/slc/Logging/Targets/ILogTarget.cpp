#include "ILogTarget.h"

#include "slc/Common/Profiling.h"

#include <ranges>

namespace slc {

	void ILogTarget::WriteTarget(std::span<MessageEntry> data, std::vector<char>& buffer)
	{
		SLC_PROFILE_FUNCTION();

		auto filtered_data = data | std::views::filter([this](auto const& entry) { return ShouldWriteMessage(entry); });

		std::size_t size{};
		for (auto const& entry : filtered_data)
		{
			std::memcpy(buffer.data() + size, entry.message.data(), entry.length);
			size += entry.length;

			buffer[size] = '\n';
			size += 1;
		}

		if (size == 0)
			return;

		DoWriteTarget(buffer, size);
	}
}
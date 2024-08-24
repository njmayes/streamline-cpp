#pragma once

#include <string_view>

namespace slc {

	struct IComboEntry
	{
		std::string_view key;

		constexpr IComboEntry(std::string_view label) : key(label) {}
		virtual const void* getVal() const = 0;
	};

	template<typename T>
	struct ComboEntry : public IComboEntry
	{
		T value;

		constexpr ComboEntry(std::string_view label, const T& val)
			: IComboEntry(label), value(val) {}
		bool operator==(const ComboEntry<T>& other) const { return value == other.value; }
		auto operator<=>(const ComboEntry<T>& other) const { return value <=> other.value; }

		const void* getVal() const override { return &value; }
	};
}
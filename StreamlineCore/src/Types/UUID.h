#pragma once

#include "Common/Base.h"

namespace slc {

	class UUID
	{
	public:
		UUID()
		: mID(sUniformDistribution(sEngine)) {}
        
		constexpr UUID(uint64_t id)
			: mID(id) {}

		uint64_t get() const { return mID; }

		operator bool() const { return mID != 0; }
		auto operator<=>(const UUID&) const = default;
		auto operator<=>(std::unsigned_integral auto val) const { return mID <=> val; }

		std::string to_string() const { return std::to_string(mID); }

	private:
		uint64_t mID;

        inline static std::random_device sRandomDevice;
        inline static std::mt19937_64 sEngine = std::mt19937_64(sRandomDevice());
        inline static std::uniform_int_distribution<uint64_t>(sUniformDistribution);
	};
}

namespace std {
	template<typename T> struct hash;

	template<>
	struct hash<slc::UUID>
	{
		std::size_t operator()(const slc::UUID& uuid) const
		{
			return uuid.get();;
		}
	};

}
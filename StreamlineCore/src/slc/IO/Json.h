#pragma once

#include "slc/Common/Base.h"

#include "json.hpp"

namespace slc {

	template<typename T>
	concept CanSerialise = IsStandard<T> and requires (T && t)
	{
		{ t.ToJson() } -> std::same_as<nlohmann::json>;
	};

	template<typename T>
	concept CanDeserialise = IsStandard<T> and requires
	{
		{ T::FromJson(std::declval<nlohmann::json>()) } -> std::same_as<T>;
	};

	template<typename T>
	concept Serialisable = CanSerialise<T> and CanDeserialise<T>;

	class JSON
	{
	public:
		template<Serialisable T>
		static void Serialise(const T& value, const fs::path& filepath)
		{
			std::ofstream f(filepath);
			nlohmann::json j = value.ToJson();
			f << std::setw(4) << j << std::endl;
		}

		template<Serialisable T>
		static T Deserialise(const fs::path& filepath)
		{
			std::ifstream f(filepath);
			nlohmann::json data = nlohmann::json::parse(f);
			return T::FromJson(data);
		}
	};

}


#define SLC_JSON_SERIALISE_MEMBER(x) data[#x] = this->x;
#define SLC_JSON_DESERIALISE_MEMBER(x) value.x = data[#x].template get<typeof(value.x)>();

#define SLC_JSON_SERIALISE(CLASS, ...)							\
	nlohmann::json ToJson()	const								\
	{															\
		nlohmann::json data{};									\
		SLC_FOR_EACH(SLC_JSON_SERIALISE_MEMBER, __VA_ARGS__)	\
		return data;											\
	}															\
	static CLASS FromJson(const nlohmann::json& data)			\
	{															\
		CLASS value{};											\
		SLC_FOR_EACH(SLC_JSON_DESERIALISE_MEMBER, __VA_ARGS__)	\
		return std::move(value);								\
	}
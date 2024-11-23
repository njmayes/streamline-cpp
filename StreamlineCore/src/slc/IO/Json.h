#pragma once

#include "slc/Common/Base.h"

#include "json.hpp"

namespace slc::JSON {

	template<typename T>
	concept CanSerialise = requires (const T& t)
	{
		{ to_json(std::declval<nlohmann::json&>(), t) };
	};

	template<typename T>
	concept CanDeserialise = requires (T& t)
	{
		{ from_json(std::declval<const nlohmann::json&>(), t) };
	};

	template<typename T>
	concept Serialisable = CanSerialise<T> and CanDeserialise<T>;

	template<Serialisable T>
	inline void Serialise(const T& value, std::string_view filepath)
	{
		std::ofstream f(filepath.data());
		f << std::setw(4) << nlohmann::json::parse(value) << std::endl;
	}

	template<Serialisable T>
	inline T Deserialise(std::string_view filepath)
	{
		std::ifstream f(filepath.data());
		nlohmann::json data = nlohmann::json::parse(f);
		return data.template get<T>();
	}
}

#define SLC_JSON_SERIALISE(CLASS, ...) NLOHMANN_DEFINE_TYPE_INTRUSIVE(CLASS, __VA_ARGS__)
#define SLC_JSON_SERIALISE_ENUM(ENUM_TYPE)														\
    template<typename BasicJsonType>                                                            \
    inline void to_json(BasicJsonType& j, const ENUM_TYPE& e)                                   \
    {                                                                                           \
        static_assert(std::is_enum<ENUM_TYPE>::value, #ENUM_TYPE " must be an enum!");          \
        static const auto m = magic_enum::enum_entries<ENUM_TYPE>();							\
        auto it = std::find_if(std::begin(m), std::end(m),                                      \
                               [e](const std::pair<ENUM_TYPE, BasicJsonType>& ej_pair) -> bool  \
        {                                                                                       \
            return ej_pair.first == e;                                                          \
        });                                                                                     \
        j = ((it != std::end(m)) ? it : std::begin(m))->second;                                 \
    }                                                                                           \
    template<typename BasicJsonType>                                                            \
    inline void from_json(const BasicJsonType& j, ENUM_TYPE& e)                                 \
    {                                                                                           \
        static_assert(std::is_enum<ENUM_TYPE>::value, #ENUM_TYPE " must be an enum!");          \
        static const auto m = magic_enum::enum_entries<ENUM_TYPE>();							\
        auto it = std::find_if(std::begin(m), std::end(m),                                      \
                               [&j](const std::pair<ENUM_TYPE, BasicJsonType>& ej_pair) -> bool \
        {                                                                                       \
            return ej_pair.second == j;                                                         \
        });                                                                                     \
        e = ((it != std::end(m)) ? it : std::begin(m))->first;                                  \
    }
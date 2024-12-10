#pragma once

#include "Core.h"

#include <exception>
#include <sstream>

namespace slc {

    class BadReflectionCastException : public std::exception
    {
    public:
        BadReflectionCastException(std::string_view target, std::string_view actual)
        {
            msg = std::format("Attempted to convert reflected type to an invalid value. Target type was {} but actual type was {}", target, actual);
        }

        const char* what() const noexcept override
        {
            return msg.data();
        }

    private:
        std::string msg;
    };


    template<typename... Args>
    class UnreflectedTargetException : public std::exception
    {
    public:
        UnreflectedTargetException(std::string_view type)
        {
            std::stringstream ss;
            (ss << ... << (std::format("{}, ", TypeTraits<Args>::Name)));

            auto args = ss.str();
            if constexpr (sizeof...(Args) > 0)
                args = args.substr(0, args.size() - 2); // Remove extra comma and space

            msg = std::format("Attempted to target type {} for reflection when required data has not been reflected. [Args: {}]", type, args);
        }

        const char* what() const noexcept override
        {
            return msg.data();
        }

    private:
        std::string msg;
    };
}
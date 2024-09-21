#pragma once

#include "Core.h"

#include <exception>
#include <string>

namespace slc {

    class BadReflectionCastException : public std::exception
    {
    public:
        BadReflectionCastException(std::string_view target, std::string_view actual)
        {
            msg = std::format("Attempted to convert reflected type to an invalid value. Target type was {} but actual type was {}", target, actual);
        }

        const char* what() const override
        {
            return msg.c_str();
        }

    private:
        std::string msg;
    };
}
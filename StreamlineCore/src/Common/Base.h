#pragma once

#include "Macros.h"
#include "Reflection.h"
#include "Environment.h"
#include "Memory.h"

#include <filesystem>
#include <format>
#include <fstream>

namespace slc {

    using Byte = uint8_t;

    template<typename TResult, typename... TArgs>
    using Func = std::function<TResult(TArgs...)>;

    template<typename... TArgs>
    using Action = Func<void, TArgs...>;

    template<typename... T>
    using Predicate = Func<bool, T...>;


    template<std::integral T>
    struct Limits
    {
        SCONSTEXPR T Min     = std::numeric_limits<T>::min();
        SCONSTEXPR T Max     = std::numeric_limits<T>::max();
        SCONSTEXPR T Epsilon = std::numeric_limits<T>::epsilon();
    };

    SCONSTEXPR size_t MakeBit(int bit) { return 1ull << bit; }

    namespace fs = std::filesystem;
}
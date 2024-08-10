#pragma once

#include "Macros.h"
#include "Reflection.h"
#include "Environment.h"

#include <memory>
#include <filesystem>
#include <format>

#include <cassert>

namespace slc {

    using Byte = uint8_t;

    template<typename T>
    using Impl = std::unique_ptr<T>;

    template<typename T, typename... Args>
    inline static constexpr Impl<T> MakeImpl(Args&&... args) { return std::make_unique<T>(std::forward<Args>(args)...); }

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
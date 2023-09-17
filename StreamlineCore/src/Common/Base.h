#pragma once

#include "Std.h"
#include "Platform.h"
#include "Reflection.h"
#include "System.h"

namespace slc {

#define SLC_EXPAND_MACRO(x) x

#define ASSERT(x, ...)  assert(x)
#define SASSERT(x, ...) static_assert(x)

#define SCONSTEXPR static constexpr
#define SCONSTEVAL static consteval

#define ifc if constexpr

#define typeof(T) std::decay_t<decltype(T)>

#define LOG(...)

    template<typename T>
    using Impl = std::unique_ptr<T>;
    template<typename T>
    using Ref = std::shared_ptr<T>;

    template<typename T, typename... Args>
    inline static Impl<T> MakeImpl(Args&&... args) { return std::make_unique<T>(std::forward<Args>(args)...); }
    template<typename T, typename... Args>
    inline static Ref<T> MakeRef(Args&&... args) { return std::make_shared<T>(std::forward<Args>(args)...); }

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

    SCONSTEXPR size_t MakeBit(int bit) { return 1 << bit; }

    namespace fs = std::filesystem;
}
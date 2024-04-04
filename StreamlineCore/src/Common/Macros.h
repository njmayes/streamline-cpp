#pragma once

#include "Platform.h"

#define SLC_EXPAND_MACRO(x) x

#define ASSERT(x, ...)  assert(x)
#define SASSERT(x, ...) static_assert(x)

#define SCONSTEXPR static constexpr
#define SCONSTEVAL static consteval

#define typeof(T) std::remove_reference_t<decltype(T)>

#define CONCAT_1(a)									a
#define CONCAT_2(a,b)								a, b
#define CONCAT_3(a,b,c)								a, b, c
#define CONCAT_4(a,b,c,d)							a, b, c, d
#define GET_LEVEL(_1, _2, _3, _4, LEVEL_N, ...)		LEVEL_N
#define EXPAND_TEMPLATE(...) SLC_EXPAND_MACRO(GET_LEVEL(__VA_ARGS__, CONCAT_4, CONCAT_3, CONCAT_2, CONCAT_1)(__VA_ARGS__))

#define LOG(...) // TODO
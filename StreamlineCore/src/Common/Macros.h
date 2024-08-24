#pragma once

#include "Platform.h"

#include <cassert>

#define SLC_EXPAND_MACRO(x) x

#ifdef SLC_DEBUG
	#define ASSERT(x, ...)  assert(x)
#else
	#define ASSERT(...) [[assume(x)]]
#endif

#define SASSERT(x, ...) static_assert(x)

#define SCONSTEXPR static constexpr
#define SCONSTEVAL static consteval

#define typeof(T) std::remove_cvref_t<decltype(T)>

#define CONCAT_1(a)									a
#define CONCAT_2(a,b)								a, b
#define CONCAT_3(a,b,c)								a, b, c
#define CONCAT_4(a,b,c,d)							a, b, c, d
#define GET_LEVEL(_1, _2, _3, _4, LEVEL_N, ...)		LEVEL_N
#define EXPAND_TEMPLATE(...) SLC_EXPAND_MACRO(GET_LEVEL(__VA_ARGS__, CONCAT_4, CONCAT_3, CONCAT_2, CONCAT_1)(__VA_ARGS__))

#define LOG(...) // TODO



// Helper macros to handle the enumeration
#define SLC_GET_ARG_COUNT(...)  SLC_GET_ARG_COUNT_IMPL(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define SLC_GET_ARG_COUNT_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, count, ...) count

#define SLC_FOR_EACH_1(what, x) what(x)
#define SLC_FOR_EACH_2(what, x, ...) what(x); SLC_FOR_EACH_1(what, __VA_ARGS__)
#define SLC_FOR_EACH_3(what, x, ...) what(x); SLC_FOR_EACH_2(what, __VA_ARGS__)
#define SLC_FOR_EACH_4(what, x, ...) what(x); SLC_FOR_EACH_3(what, __VA_ARGS__)
#define SLC_FOR_EACH_5(what, x, ...) what(x); SLC_FOR_EACH_4(what, __VA_ARGS__)
#define SLC_FOR_EACH_6(what, x, ...) what(x); SLC_FOR_EACH_5(what, __VA_ARGS__)
#define SLC_FOR_EACH_7(what, x, ...) what(x); SLC_FOR_EACH_6(what, __VA_ARGS__)
#define SLC_FOR_EACH_8(what, x, ...) what(x); SLC_FOR_EACH_7(what, __VA_ARGS__)
#define SLC_FOR_EACH_9(what, x, ...) what(x); SLC_FOR_EACH_8(what, __VA_ARGS__)
#define SLC_FOR_EACH_10(what, x, ...) what(x); SLC_FOR_EACH_9(what, __VA_ARGS__)

// Select the right macro based on the number of arguments
#define SLC_FOR_EACH_NARG_IMPL(count) SLC_FOR_EACH_##count
#define SLC_FOR_EACH_NARG(count) SLC_FOR_EACH_NARG_IMPL(count)
#define SLC_FOR_EACH(what, ...) SLC_FOR_EACH_NARG(SLC_GET_ARG_COUNT(__VA_ARGS__))(what, __VA_ARGS__)
#pragma once

#include "Platform.h"

#include <cassert>

#define SLC_EXPAND_MACRO(x) x

#ifdef SLC_DEBUG
	#define ASSERT(x, ...)  assert(x)
#else
	#define ASSERT(...) [[assume(x)]]
#endif

#define SLC_STRINGIFY( L )  #L 
#define SLC_MAKE_STRING( x ) SLC_STRINGIFY(x)
#define SLC_TODO(x) __pragma(message(__FILE__ "(" SLC_MAKE_STRING(__LINE__) ") : TODO - " x))

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



#define SLC_PARENS ()

// Rescan macro tokens 256 times
#define SLC_EXPAND(arg)  SLC_EXPAND1(SLC_EXPAND1(SLC_EXPAND1(SLC_EXPAND1(arg))))
#define SLC_EXPAND1(arg) SLC_EXPAND2(SLC_EXPAND2(SLC_EXPAND2(SLC_EXPAND2(arg))))
#define SLC_EXPAND2(arg) SLC_EXPAND3(SLC_EXPAND3(SLC_EXPAND3(SLC_EXPAND3(arg))))
#define SLC_EXPAND3(arg) SLC_EXPAND4(SLC_EXPAND4(SLC_EXPAND4(SLC_EXPAND4(arg))))
#define SLC_EXPAND4(arg) SLC_EXPAND_MACRO(arg)

#define SLC_FOR_EACH(macro, ...)                                    \
  __VA_OPT__(SLC_EXPAND(SLC_FOR_EACH_HELPER(macro, __VA_ARGS__)))
#define SLC_FOR_EACH_HELPER(macro, a1, ...)                         \
  macro(a1)                                                     \
  __VA_OPT__(SLC_FOR_EACH_AGAIN SLC_PARENS (macro, __VA_ARGS__))
#define SLC_FOR_EACH_AGAIN() SLC_FOR_EACH_HELPER
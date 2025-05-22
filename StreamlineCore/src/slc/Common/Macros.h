#pragma once

#include "Platform.h"

#include <cassert>

#define SLC_EXPAND_MACRO(x) x

#ifdef __has_cpp_attribute
	#define SLC_HAS_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
	#define SLC_HAS_ATTRIBUTE(x)
#endif


#ifdef SLC_DEBUG
	#define ASSERT(x, ...) assert(x)
#else
	#if SLC_HAS_ATTRIBUTE(assume)
		#define ASSERT(...) [[assume(x)]]
	#else
		#if defined(SLC_COMPILER_MSVC)
			#define ASSERT(x, ...)  __assume(x)
		#elif defined(SLC_COMPILER_GCC)	
			#define ASSERT(x, ...)  __attribute__((assume(x)))
		#elif defined(SLC_COMPILER_CLANG)	
			#define ASSERT(x, ...)  __builtin_assume(x)
		#else
			#error "No assume attribute"
		#endif // SLC_PLATFORM_LINUX
	#endif // __has_cpp_attribute(assume)
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



#define SLC_PARENS ()

// Rescan macro tokens 256 times
#define SLC_EXPAND(arg)  SLC_EXPAND1(SLC_EXPAND1(SLC_EXPAND1(SLC_EXPAND1(arg))))
#define SLC_EXPAND1(arg) SLC_EXPAND2(SLC_EXPAND2(SLC_EXPAND2(SLC_EXPAND2(arg))))
#define SLC_EXPAND2(arg) SLC_EXPAND3(SLC_EXPAND3(SLC_EXPAND3(SLC_EXPAND3(arg))))
#define SLC_EXPAND3(arg) SLC_EXPAND4(SLC_EXPAND4(SLC_EXPAND4(SLC_EXPAND4(arg))))
#define SLC_EXPAND4(arg) SLC_EXPAND_MACRO(arg)
#define SLC_EVAL( ... ) SLC_EVAL1024( __VA_ARGS__ )
#define SLC_EVAL1024( ... ) SLC_EVAL512( SLC_EVAL512( __VA_ARGS__ ) )
#define SLC_EVAL512( ... ) SLC_EVAL256( SLC_EVAL256( __VA_ARGS__ ) )
#define SLC_EVAL256( ... ) SLC_EVAL128( SLC_EVAL128( __VA_ARGS__ ) )
#define SLC_EVAL128( ... ) SLC_EVAL64( SLC_EVAL64( __VA_ARGS__ ) )
#define SLC_EVAL64( ... ) SLC_EVAL32( SLC_EVAL32( __VA_ARGS__ ) )
#define SLC_EVAL32( ... ) SLC_EVAL16( SLC_EVAL16( __VA_ARGS__ ) )
#define SLC_EVAL16( ... ) SLC_EVAL8( SLC_EVAL8( __VA_ARGS__ ) )
#define SLC_EVAL8( ... ) SLC_EVAL4( SLC_EVAL4( __VA_ARGS__ ) )
#define SLC_EVAL4( ... ) SLC_EVAL2( SLC_EVAL2( __VA_ARGS__ ) )
#define SLC_EVAL2( ... ) SLC_EVAL1( SLC_EVAL1( __VA_ARGS__ ) )
#define SLC_EVAL1( ... ) __VA_ARGS__

#define SLC_EMPTY()
#define SLC_DEFER( id ) id SLC_EMPTY()
#define SLC_OBSTRUCT( ... ) __VA_ARGS__ SLC_DEFER( SLC_EMPTY )()

#define SLC_FOR_EACH_SEP( macro, sep, ... ) \
	SLC_EVAL( SLC_FOR_EACH_SEP_INNER( macro, sep, __VA_ARGS__ ) )

#define SLC_FOR_EACH_SEP_INNER( macro, sep, a1, ... ) \
	macro( a1 )                                       \
	__VA_OPT__( SLC_OBSTRUCT( SLC_FOR_EACH_SEP_CONTINUE )()( macro, sep, __VA_ARGS__ ) )

#define SLC_FOR_EACH_SEP_CONTINUE() SLC_FOR_EACH_SEP_INNER_NEXT
#define SLC_FOR_EACH_SEP_INNER_NEXT( macro, sep, a1, ... ) \
	sep() macro( a1 )                                      \
	__VA_OPT__( SLC_OBSTRUCT( SLC_FOR_EACH_SEP_CONTINUE )()( macro, sep, __VA_ARGS__ ) )

// Separator macro example:
#define SLC_COMMA() ,
#define SLC_NONE()


#define SLC_FOR_EACH(macro, ...) SLC_FOR_EACH_SEP(macro, SLC_NONE, __VA_ARGS__)
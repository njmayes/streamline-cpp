#pragma once

#include "Platform.h"

#include <cassert>

#define SLC_VERSION "0.1.0"

#define SLC_EXPAND_MACRO( x ) x

#define SLC_CONCAT( a, b ) SLC_CONCAT_IMPL( a, b )
#define SLC_CONCAT_IMPL( a, b ) a##b

#ifdef __has_cpp_attribute
#define SLC_HAS_ATTRIBUTE( x ) __has_cpp_attribute( x )
#else
#define SLC_HAS_ATTRIBUTE( x )
#endif


#ifdef SLC_DEBUG
#define ASSERT( x, ... ) assert( x )
#else
#if SLC_HAS_ATTRIBUTE( assume )
#define ASSERT( x, ... ) [[assume( x )]]
#else
#if defined( SLC_COMPILER_MSVC )
#define ASSERT( x, ... ) __assume( x )
#elif defined( SLC_COMPILER_GCC )
#define ASSERT( x, ... ) __attribute__( ( assume( x ) ) )
#elif defined( SLC_COMPILER_CLANG )
#define ASSERT( x, ... ) __builtin_assume( x )
#else
#error "No assume attribute"
#endif // SLC_PLATFORM_LINUX
#endif // __has_cpp_attribute(assume)
#endif

#if defined( SLC_COMPILER_MSVC )
#define SLC_INLINE __forceinline
#define SLC_RESTRICT __restrict
#elif defined( SLC_COMPILER_GCC )
#define SLC_INLINE [[gnu::always_inline]]
#define SLC_RESTRICT __restrict
#elif defined( SLC_COMPILER_CLANG )
#define SLC_INLINE [[clang::always_inline]]
#define SLC_RESTRICT __restrict
#else
#define SLC_INLINE
#endif

#define SLC_STRINGIFY( L ) #L
#define SLC_MAKE_STRING( x ) SLC_STRINGIFY( x )

#if defined( SLC_COMPILER_MSVC )
#define SLC_TODO( x ) __pragma( message( __FILE__ "(" SLC_MAKE_STRING( __LINE__ ) ") : TODO - " x ) )
#elif defined( SLC_COMPILER_GCC ) || defined( __clang__ )
#define SLC_TODO( x ) _Pragma( SLC_MAKE_STRING( message( __FILE__ ":" SLC_MAKE_STRING( __LINE__ ) " TODO - " x ) ) )
#else
#define SLC_TODO( x )
#endif


#define SASSERT( x, ... ) static_assert( x )

#define SCONSTEXPR static constexpr
#define SCONSTEVAL static consteval

#define typeof( T ) std::remove_cvref_t< decltype( T ) >

#define SLC_PARENS ()

// Rescan macro tokens 256 times
#define SLC_EXPAND( arg ) SLC_EXPAND1( SLC_EXPAND1( SLC_EXPAND1( SLC_EXPAND1( arg ) ) ) )
#define SLC_EXPAND1( arg ) SLC_EXPAND2( SLC_EXPAND2( SLC_EXPAND2( SLC_EXPAND2( arg ) ) ) )
#define SLC_EXPAND2( arg ) SLC_EXPAND3( SLC_EXPAND3( SLC_EXPAND3( SLC_EXPAND3( arg ) ) ) )
#define SLC_EXPAND3( arg ) SLC_EXPAND4( SLC_EXPAND4( SLC_EXPAND4( SLC_EXPAND4( arg ) ) ) )
#define SLC_EXPAND4( arg ) SLC_EXPAND_MACRO( arg )

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

#define SLC_NARGS_IMPL(                                      \
  _0,_1,_2,_3,_4,_5,_6,_7,_8,_9,                             \
  _10,_11,_12,_13,_14,_15,_16,_17,_18,_19,                   \
  _20,_21,_22,_23,_24,_25,_26,_27,_28,_29,                   \
  _30,_31,_32,_33,_34,_35,_36,_37,_38,_39,                   \
  _40,_41,_42,_43,_44,_45,_46,_47,_48,_49,                   \
  _50,_51,_52,_53,_54,_55,_56,_57,_58,_59,                   \
  _60,_61,_62,_63,_64,_65,_66,_67,_68,_69,                   \
  _70,_71,_72,_73,_74,_75,_76,_77,_78,_79,                   \
  _80,_81,_82,_83,_84,_85,_86,_87,_88,_89,                   \
  _90,_91,_92,_93,_94,_95,_96,_97,_98,_99,                   \
  _100,_101,_102,_103,_104,_105,_106,_107,_108,_109,         \
  _110,_111,_112,_113,_114,_115,_116,_117,_118,_119,         \
  _120,_121,_122,_123,_124,_125,_126,_127, N, ...) N

#define SLC_NARGS_REVERSE_128                                                                 \
	127, 126, 125, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112, 111, 110, \
		109, 108, 107, 106, 105, 104, 103, 102, 101, 100, 99, 98, 97, 96, 95, 94, 93, 92,     \
		91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74,               \
		73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56,               \
		55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38,               \
		37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20,               \
		19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2,                       \
		1, 0

#define SLC_NARGS_( ... ) SLC_NARGS_IMPL( __VA_ARGS__ )
#define SLC_NARGS( ... ) SLC_NARGS_( __VA_ARGS__, SLC_NARGS_REVERSE_128 )

#define SLC_TUPLE_COUNT_I( ... ) SLC_NARGS( __VA_ARGS__ )
#define SLC_TUPLE_COUNT( P ) SLC_TUPLE_COUNT_I SLC_EXPAND_MACRO( P )


#define SLC_I0 ()
#define SLC_I_NEXT( P ) SLC_I_NEXT_I P
#define SLC_I_NEXT_I( ... ) ( __VA_ARGS__, ~)

#define SLC_I_VALUE( P ) SLC_TUPLE_COUNT( P )

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


#define SLC_FOR_EACH_I_SEP( macro, sep, ... ) \
	SLC_EVAL( SLC_FOR_EACH_I_SEP_INNER( macro, sep, SLC_I0, __VA_ARGS__ ) )

#define SLC_FOR_EACH_I_SEP_INNER( macro, sep, i_state, a1, ... )   \
	macro( SLC_I_VALUE( i_state ), a1 )                            \
		__VA_OPT__( SLC_OBSTRUCT( SLC_FOR_EACH_I_SEP_CONTINUE )()( \
			macro, sep, SLC_I_NEXT( i_state ), __VA_ARGS__         \
		) )

#define SLC_FOR_EACH_I_SEP_CONTINUE() SLC_FOR_EACH_I_SEP_INNER_NEXT
#define SLC_FOR_EACH_I_SEP_INNER_NEXT( macro, sep, i_state, a1, ... ) \
	sep() macro( SLC_I_VALUE( i_state ), a1 )                         \
		__VA_OPT__( SLC_OBSTRUCT( SLC_FOR_EACH_I_SEP_CONTINUE )()(    \
			macro, sep, SLC_I_NEXT( i_state ), __VA_ARGS__            \
		) )


// Separator macro example:
#define SLC_COMMA() ,
#define SLC_NONE()


#define SLC_FOR_EACH( macro, ... ) SLC_FOR_EACH_SEP( macro, SLC_NONE, __VA_ARGS__ )

#define SLC_FOR_EACH_I( macro, ... ) SLC_FOR_EACH_I_SEP( macro, SLC_NONE, __VA_ARGS__ )

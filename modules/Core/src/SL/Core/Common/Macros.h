#pragma once

#include "Platform.h"
#include "Error.h"

// Hacky macros
#define SL_STRINGIFY( L ) #L
#define SL_MAKE_STRING( x ) SL_STRINGIFY( x )

#define SL_EXPAND_MACRO( x ) x

#define SL_CONCAT( a, b ) SL_CONCAT_IMPL( a, b )
#define SL_CONCAT_IMPL( a, b ) a##b


// Versioning
#define SL_VERSION_MAJOR 1
#define SL_VERSION_MINOR 0
#define SL_VERSION_PATCH 0

#define SL_VERSION SL_MAKE_STRING( SL_VERSION_MAJOR.SL_VERSION_MINOR.SL_VERSION_PATCH )


// Attributes
#ifdef __has_cpp_attribute
#define SL_HAS_ATTRIBUTE( x ) __has_cpp_attribute( x )
#else
#define SL_HAS_ATTRIBUTE( x )
#endif


// Assertion / validation macros:
//
// SL_ASSERT(x)
//   - Debug: runtime check (assert).
//   - Release: treated as an assumption (UB if false).
//   - Use only for invariants that must always be true and have no side effects.
//
// SL_VERIFY(x, msg)
//   - Debug + Release: always evaluates and checks the condition.
//   - On failure: reports error (with message + source location) and aborts.
//   - Use when the condition must be validated in production or has side effects.
//
// SL_FATAL_ERROR(msg)
//   - Debug + Release: always triggers a fatal error.
//   - Reports message + source location, then aborts.
//   - Use for unreachable code paths or unrecoverable states.

#define SL_DETAIL_GET_MESSAGE_MACRO( _1, _2, NAME, ... ) NAME
#define SL_DETAIL_MESSAGE_OR_NULLPTR( ... ) \
	SL_DETAIL_MESSAGE_OR_NULLPTR_IMPL( __VA_OPT__( __VA_ARGS__, ) nullptr )

#define SL_DETAIL_MESSAGE_OR_NULLPTR_IMPL( ... ) \
	SL_DETAIL_MESSAGE_OR_NULLPTR_SELECT( __VA_ARGS__ )

#define SL_DETAIL_MESSAGE_OR_NULLPTR_SELECT( value, ... ) value

#define SL_FATAL_ERROR( ... )                           \
	do                                                  \
	{                                                   \
		::sl::detail::ReportFatalError(                 \
			"Fatal Error",                              \
			nullptr,                                    \
			SL_DETAIL_MESSAGE_OR_NULLPTR( __VA_ARGS__ ) \
		);                                              \
	} while ( false )

#define SL_VERIFY( x, ... )                                 \
	do                                                      \
	{                                                       \
		if ( !( x ) )                                       \
		{                                                   \
			::sl::detail::ReportFatalError(                 \
				"Verify Failed",                            \
				#x,                                         \
				SL_DETAIL_MESSAGE_OR_NULLPTR( __VA_ARGS__ ) \
			);                                              \
		}                                                   \
	} while ( false )

#ifdef SL_DEBUG

#define SL_ASSERT( x, ... )                                 \
	do                                                      \
	{                                                       \
		if ( !( x ) )                                       \
		{                                                   \
			::sl::detail::ReportFatalError(                 \
				"Assert Failed",                            \
				#x,                                         \
				SL_DETAIL_MESSAGE_OR_NULLPTR( __VA_ARGS__ ) \
			);                                              \
		}                                                   \
	} while ( false )

#else

#if SL_HAS_ATTRIBUTE( assume )
#define SL_ASSERT( x, ... ) [[assume( x )]]
#elif defined( SL_COMPILER_MSVC )
#define SL_ASSERT( x, ... ) __assume( x )
#elif defined( SL_COMPILER_CLANG )
#define SL_ASSERT( x, ... ) __builtin_assume( x )
#elif defined( SL_COMPILER_GCC )
#define SL_ASSERT( x, ... ) __attribute__( ( assume( x ) ) )
#else
#error "No assume support for SL_ASSERT on this compiler"
#endif

#endif


// Force inline and restrict

#if defined( SL_COMPILER_MSVC )
#define SL_INLINE __forceinline
#define SL_RESTRICT __restrict
#elif defined( SL_COMPILER_GCC )
#define SL_INLINE [[gnu::always_inline]]
#define SL_RESTRICT __restrict
#elif defined( SL_COMPILER_CLANG )
#define SL_INLINE [[clang::always_inline]]
#define SL_RESTRICT __restrict
#else
#define SL_INLINE
#define SL_RESTRICT
#endif


// Static assertions
#define SL_COMPILE_CHECK( cond, func, err ) static_assert( cond, "\n\n\t[" SL_STRINGIFY( func ) "] Error: " err "\n" )
#define SL_COMPILE_ERROR_IF( cond, func, err ) SL_COMPILE_CHECK( not( cond ), func, err )
#define SL_COMPILE_ERROR( func, err ) SL_COMPILE_CHECK( false, func, err )


// TODO macro

#if defined( SL_COMPILER_MSVC )
#define SL_TODO( x ) __pragma( message( __FILE__ "(" SL_MAKE_STRING( __LINE__ ) ") : TODO - " x ) )
#elif defined( SL_COMPILER_GCC ) || defined( __clang__ )
#define SL_TODO( x ) _Pragma( SL_MAKE_STRING( message( __FILE__ ":" SL_MAKE_STRING( __LINE__ ) " TODO - " x ) ) )
#else
#define SL_TODO( x )
#endif


// typeof macro
#define SL_TYPEOF( x ) std::remove_cvref_t< decltype( x ) >


// Separator macros, for use below
#define SL_COMMA() ,
#define SL_NONE()


// Variadic macro utilities for iterating over argument lists.
//
// These macros allow you to apply another macro to each argument in a
// comma-separated list, optionally inserting separators and/or providing
// an index.
//
// ----------------------------------------------------------------------------
//
// SL_FOR_EACH(macro, a, b, c)
//   Expands to:
//     macro(a) macro(b) macro(c)
//
//   Applies `macro` to each argument in order. No separator is inserted.
//
//
// SL_FOR_EACH_I(macro, a, b, c)
//   Expands to:
//     macro(0, a) macro(1, b) macro(2, c)
//
//   Same as SL_FOR_EACH, but also passes a zero-based index as the first
//   argument to `macro`.
//
//
// SL_FOR_EACH_SEP(macro, sep, a, b, c)
//   Expands to:
//     macro(a) sep() macro(b) sep() macro(c)
//
//   Applies `macro` to each argument and inserts `sep()` between elements.
//   `sep` must be a function like macro taking no arguments (e.g. SL_COMMA).
//
//
// SL_FOR_EACH_I_SEP(macro, sep, a, b, c)
//   Expands to:
//     macro(0, a) sep() macro(1, b) sep() macro(2, c)
//
//   Indexed version of SL_FOR_EACH_SEP.

#define SL_PARENS ()

#define SL_EVAL( ... ) SL_EVAL1024( __VA_ARGS__ )
#define SL_EVAL1024( ... ) SL_EVAL512( SL_EVAL512( __VA_ARGS__ ) )
#define SL_EVAL512( ... ) SL_EVAL256( SL_EVAL256( __VA_ARGS__ ) )
#define SL_EVAL256( ... ) SL_EVAL128( SL_EVAL128( __VA_ARGS__ ) )
#define SL_EVAL128( ... ) SL_EVAL64( SL_EVAL64( __VA_ARGS__ ) )
#define SL_EVAL64( ... ) SL_EVAL32( SL_EVAL32( __VA_ARGS__ ) )
#define SL_EVAL32( ... ) SL_EVAL16( SL_EVAL16( __VA_ARGS__ ) )
#define SL_EVAL16( ... ) SL_EVAL8( SL_EVAL8( __VA_ARGS__ ) )
#define SL_EVAL8( ... ) SL_EVAL4( SL_EVAL4( __VA_ARGS__ ) )
#define SL_EVAL4( ... ) SL_EVAL2( SL_EVAL2( __VA_ARGS__ ) )
#define SL_EVAL2( ... ) SL_EVAL1( SL_EVAL1( __VA_ARGS__ ) )
#define SL_EVAL1( ... ) __VA_ARGS__

#define SL_NARGS_IMPL(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
	_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116, _117, _118, _119, _120, _121, _122, _123, _124, _125, _126, _127, N, ... \
) N

#define SL_NARGS_REVERSE_128                                                                  \
	127, 126, 125, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112, 111, 110, \
		109, 108, 107, 106, 105, 104, 103, 102, 101, 100, 99, 98, 97, 96, 95, 94, 93, 92,     \
		91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74,               \
		73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56,               \
		55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38,               \
		37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20,               \
		19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2,                       \
		1, 0

#define SL_NARGS_( ... ) SL_NARGS_IMPL( __VA_ARGS__ )
#define SL_NARGS( ... ) SL_NARGS_( __VA_ARGS__, SL_NARGS_REVERSE_128 )

#define SL_TUPLE_COUNT_I( ... ) SL_NARGS( __VA_ARGS__ )
#define SL_TUPLE_COUNT( P ) SL_TUPLE_COUNT_I SL_EXPAND_MACRO( P )


#define SL_I0 ()
#define SL_I_NEXT( P ) SL_I_NEXT_I P
#define SL_I_NEXT_I( ... ) ( __VA_ARGS__, ~)

#define SL_I_VALUE( P ) SL_TUPLE_COUNT( P )

#define SL_EMPTY()
#define SL_DEFER( id ) id SL_EMPTY()
#define SL_OBSTRUCT( ... ) __VA_ARGS__ SL_DEFER( SL_EMPTY )()


#define SL_FOR_EACH_SEP( macro, sep, ... ) \
	SL_EVAL( SL_FOR_EACH_SEP_INNER( macro, sep, __VA_ARGS__ ) )

#define SL_FOR_EACH_SEP_INNER( macro, sep, a1, ... ) \
	macro( a1 )                                      \
		__VA_OPT__( SL_OBSTRUCT( SL_FOR_EACH_SEP_CONTINUE )()( macro, sep, __VA_ARGS__ ) )

#define SL_FOR_EACH_SEP_CONTINUE() SL_FOR_EACH_SEP_INNER_NEXT
#define SL_FOR_EACH_SEP_INNER_NEXT( macro, sep, a1, ... ) \
	sep() macro( a1 )                                     \
		__VA_OPT__( SL_OBSTRUCT( SL_FOR_EACH_SEP_CONTINUE )()( macro, sep, __VA_ARGS__ ) )


#define SL_FOR_EACH_I_SEP( macro, sep, ... ) \
	SL_EVAL( SL_FOR_EACH_I_SEP_INNER( macro, sep, SL_I0, __VA_ARGS__ ) )

#define SL_FOR_EACH_I_SEP_INNER( macro, sep, i_state, a1, ... )  \
	macro( SL_I_VALUE( i_state ), a1 )                           \
		__VA_OPT__( SL_OBSTRUCT( SL_FOR_EACH_I_SEP_CONTINUE )()( \
			macro, sep, SL_I_NEXT( i_state ), __VA_ARGS__        \
		) )

#define SL_FOR_EACH_I_SEP_CONTINUE() SL_FOR_EACH_I_SEP_INNER_NEXT
#define SL_FOR_EACH_I_SEP_INNER_NEXT( macro, sep, i_state, a1, ... ) \
	sep() macro( SL_I_VALUE( i_state ), a1 )                         \
		__VA_OPT__( SL_OBSTRUCT( SL_FOR_EACH_I_SEP_CONTINUE )()(     \
			macro, sep, SL_I_NEXT( i_state ), __VA_ARGS__            \
		) )


#define SL_FOR_EACH( macro, ... ) __VA_OPT__( SL_FOR_EACH_SEP( macro, SL_NONE, __VA_ARGS__ ) )

#define SL_FOR_EACH_I( macro, ... ) __VA_OPT__( SL_FOR_EACH_I_SEP( macro, SL_NONE, __VA_ARGS__ ) )

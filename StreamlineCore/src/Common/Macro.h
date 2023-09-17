#pragma once

#pragma region Common

    #define ASSERT(x, ...)  assert(x)
    #define SASSERT(x, ...) static_assert(x)

    #define SCONSTEXPR static constexpr
    #define SCONSTEVAL static consteval

    #define ifc if constexpr

    #define typeof(T) std::decay_t<decltype(T)>

    #define LOG(...)

#pragma endregion

#pragma region Platform

    // Platform detection using predefined macros
    #ifdef _WIN32
        /* Windows x64/x86 */
        #ifdef _WIN64
        /* Windows x64  */
            #define SLC_PLATFORM_WINDOWS
        #else
        /* Windows x86 */
            #error "x86 Builds are not supported!"
        #endif
    #elif defined(__APPLE__) || defined(__MACH__)
        #include <TargetConditionals.h>
        /* TARGET_OS_MAC exists on all the platforms
        * so we must check all of them (in this order)
        * to ensure that we're running on MAC
        * and not some other Apple platform */
        #if TARGET_IPHONE_SIMULATOR == 1
            #error "IOS simulator is not supported!"
        #elif TARGET_OS_IPHONE == 1
            #define SLC_PLATFORM_IOS
            #error "IOS is not supported!"
        #elif TARGET_OS_MAC == 1
            #define SLC_PLATFORM_MACOS
            #error "MacOS is not supported!"
        #else
            #error "Unknown Apple platform!"
        #endif
    /* We also have to check __ANDROID__ before __linux__
    * since android is based on the linux kernel
    * it has __linux__ defined */
    #elif defined(__ANDROID__)
        #define SLC_PLATFORM_ANDROID
        #error "Android is not supported!"
    #elif defined(__linux__)
        #define SLC_PLATFORM_LINUX
    #else
        /* Unknown compiler/platform */
        #error "Unknown platform!"
    #endif // End of platform detection

#pragma endregion

#pragma region Reflection

    #if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
    #    define SLC_FUNC_SIGNATURE __PRETTY_FUNCTION__
    #    define SLC_FUNC_SIGNATURE_PREFIX '='
    #    define SLC_FUNC_SIGNATURE_SUFFIX ']'
    #elif defined(__DMC__) && (__DMC__ >= 0x810)
    #    define SLC_FUNC_SIGNATURE __PRETTY_FUNCTION__
    #    define SLC_FUNC_SIGNATURE_PREFIX '='
    #    define SLC_FUNC_SIGNATURE_SUFFIX ']'
    #elif (defined(__FUNCSIG__) || (_MSC_VER))
    #    define SLC_FUNC_SIGNATURE __FUNCSIG__
    #    define SLC_FUNC_SIGNATURE_PREFIX '<'
    #    define SLC_FUNC_SIGNATURE_SUFFIX '>'
    #else
    #   error SLC_FUNC_SIGNATURE "SLC_FUNC_SIGNATURE unknown!"
    #endif

#define SLC_FUNC_SIG_STRING std::string_view { std::source_location::current().function_name() }

#pragma endregion

#pragma region Matching

    #define SLC_TAG(result, value) result##value

    #define MATCH_START(result) \
                do \
                {\
                    using SLC_TAG(result, ResultType) = std::decay_t<decltype(result)>;\
                    using SLC_TAG(result, EnumUnionType) = SLC_TAG(result, ResultType)::EnumUnionType;\
                    const auto& SLC_TAG(result, _ref) = result;

    #define MATCH(result, option, code) \
                    if (SLC_TAG(result, _ref).as_enum() == SLC_TAG(result, EnumUnionType)(option)) {\
                        code \
                    } else 
    #define MATCH_A(result, code) \
                    {\
                        code \
                    }

    #define MATCH_END {}\
                } while (false);

#pragma endregion

#pragma region Do Notation

    #define FUNC_RETURN_TYPE SLC_FUNC_SIG_STRING.substr(0, SLC_FUNC_SIG_STRING.find_first_of(' ') - 1)

    #define TRY_RET(value) \
                    ifc (FUNC_RETURN_TYPE == TypeTraits<typeof(value)>::Name)\
                    {\
                        if (value.is_err())\
                            return value;\
                    }\

    #define START(DoAction, ...) DoAction(__VA_ARGS__)
    #define NEXT(DoAction, ...) [&]() -> FunctionTraits<typeof(DoAction)>::ReturnType { return DoAction(__VA_ARGS__); }

    #define DO(ResultExpression) \
                {\
                    auto finalResult = (ResultExpression);\
                    TRY_RET(finalResult);\
                }

    #define DO_R(ResultType, ResultEnum, NewVarName, ResultExpression) \
                Result<ResultType, ResultEnum> NewVarName;\
                {\
                    auto finalResult = (ResultExpression);\
                    TRY_RET(finalResult);\
                    NewVarName = std::move(finalResult); \
                }

#pragma endregion
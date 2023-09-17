#pragma once

#include "Common/Base.h"

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
                Result<ResultType, ResultEnum> NewVarName = std::invoke([]()\
                {\
                    return (ResultExpression);\
                });\
                TRY_RET(NewVarName);

#pragma endregion
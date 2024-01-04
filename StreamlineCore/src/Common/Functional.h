#pragma once

#include "Common/Base.h"

#pragma region Matching

    #define MATCH_START(result) \
        {\
            auto&& resultVal = result;\
            std::invoke([&](auto&& resultParam){\
                auto errPtr = resultParam.as_err();\
                if (resultParam.is_ok())\
                    goto _OK_MATCH;\
                \
                switch (*errPtr)\
                {

    #define MATCH_OK(code)\
                _OK_MATCH:\
                {\
                    std::invoke([](auto& value) { code; }, resultParam.as_valref());\
                    goto _END_MATCH;\
                }

    #define MATCH(option, code)\
                case option:\
                {\
                    std::invoke([](auto error) { code; }, *errPtr);\
                    break;\
                }

    #define MATCH_ALL(code)\
                default:\
                    code;\
                    break;

    #define MATCH_END \
                }\
                _END_MATCH:\
                    ;\
            }, resultVal);\
        }

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
                    auto finalResult = std::invoke([]()\
                    {\
                        return (ResultExpression);\
                    });\
                    TRY_RET(finalResult);\
                }

    #define DO_R(ResultType, ResultEnum, NewVarName, ResultExpression) \
                Result<ResultType, ResultEnum> NewVarName = std::invoke([]()\
                {\
                    return (ResultExpression);\
                });\
                TRY_RET(NewVarName);

#pragma endregion
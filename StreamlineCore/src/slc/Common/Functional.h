#pragma once

#include "slc/Common/Base.h"

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
                    if constexpr (FUNC_RETURN_TYPE == TypeTraits<typeof(value)>::Name)\
                    {\
                        if (value.is_err())\
                            return value;\
                    }\

    #define CHAIN(DoAction) [&]() -> auto { return DoAction(); }
    #define NEXT(DoAction) [&](auto&& val) -> auto { return DoAction(std::move(val)); }

#pragma endregion
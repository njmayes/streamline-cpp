#pragma once

#include "slc/Common/Base.h"

#pragma region Do Notation

    #define FUNC_RETURN_TYPE SLC_FUNC_SIG_STRING.substr(0, SLC_FUNC_SIG_STRING.find_first_of(' ') - 1)

    #define TRY_RET(value) \
                    if constexpr (FUNC_RETURN_TYPE == TypeTraits<typeof(value)>::Name)\
                    {\
                        if (value.IsError())\
                            return value;\
                    }\

    #define CHAIN(DoAction) [&]() -> auto { return DoAction(); }
    #define NEXT(DoAction) [&](auto&& val) -> auto { return DoAction(std::move(val)); }

#pragma endregion
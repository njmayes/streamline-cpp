#pragma once

#include <any>

namespace slc {
    
    class Any
    {
    public:
        Any() = default;

        template<typename T>
        Any(T&& val)
        {
            using Traits = TypeTraits<T>;
            if constexpr (Traits::IsReference)
            {
                mValue = std::ref(val);
            }
            else
            {
                mValue = val;
            }
        }

        bool HasValue() const { return mValue.has_value(); }

        template<typename T>
        T& Get()
        {
            using Traits = TypeTraits<T>;
            if constexpr (Traits::IsReference)
            {
                return std::any_cast<std::reference_wrapper<std::remove_reference_t<T>>>(mValue).get();
            }
            else
            {
                return std::any_cast<T&>(mValue);
            }
        }

    private:
        std::any mValue;
    };
}
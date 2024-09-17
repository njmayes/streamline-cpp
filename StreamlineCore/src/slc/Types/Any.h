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
            mReferenceType = Traits::IsReference;
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
            if (mReferenceType)
            {
                return std::any_cast<std::reference_wrapper<T>>(mValue).get();
            }
            else
            {
                return std::any_cast<T&>(mValue);
            }
        }

    private:
        std::any mValue;
        bool mReferenceType;
    };
}
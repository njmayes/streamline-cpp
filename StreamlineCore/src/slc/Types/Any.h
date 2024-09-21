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
        T Get()
        {
            using Traits = TypeTraits<T>;
            if constexpr (Traits::IsReference)
            {
                using BaseType = std::remove_reference_t<T>;
                using RefType = std::reference_wrapper<BaseType>;
                if constexpr (Traits::IsConst)
                {
                    // const& types may be constructible from value type, so const& cast may not always work.
                    if (auto const_ref = std::any_cast<RefType>(&mValue))
                    {
                        return const_ref->get();
                    }
                    else
                    {
                        return std::any_cast<BaseType&>(mValue);
                    }
                }
                else
                {
                    return std::any_cast<RefType>(mValue).get();
                }
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
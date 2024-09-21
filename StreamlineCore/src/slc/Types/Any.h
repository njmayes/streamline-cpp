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
            if constexpr (Traits::IsLValueReference)
            {
                mValue = std::ref(val);
            }
            else
            {
                mValue = std::move(val);
            }
        }

        bool HasValue() const { return mValue.has_value(); }

        template<typename T>
        T Get()
        {
            SLC_TODO("Simplify this");

            using Traits = TypeTraits<T>;

            if constexpr (Traits::IsReference)
            {
                if constexpr (Traits::IsRValueReference)
                {
                    using RefType = std::remove_cvref_t<T>&;
                    return std::move(std::any_cast<RefType>(mValue));
                }
                else
                {
                    using BaseType = std::remove_reference_t<T>;
                    using RefWrapperType = std::reference_wrapper<BaseType>;
                    if constexpr (Traits::IsConst)
                    {
                        // const& types may be constructible from value type, so const& std::reference_wrapper cast may not always work.
                        if (auto const_ref = std::any_cast<RefWrapperType>(&mValue))
                        {
                            return const_ref->get();
                        }
                        else
                        {
                            using RefType = std::remove_cvref_t<T>&;
                            return std::any_cast<RefType>(mValue);
                        }
                    }
                    else
                    {
                        return std::any_cast<RefWrapperType>(mValue).get();
                    }
                }
            }
            else
            {
                if (auto reference = std::any_cast<T>(&mValue))
                {
                    return std::move(*reference);
                }
                else
                {
                    using BaseType = std::remove_reference_t<T>;
                    using RefWrapperType = std::reference_wrapper<BaseType>;
                    return std::any_cast<RefWrapperType>(&mValue)->get();
                }
            }
        }

    private:
        std::any mValue;
    };
}
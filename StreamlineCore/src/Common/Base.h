#pragma once

#include "Std.h"
#include "Macro.h"

namespace slc {

    namespace Reflection {

#if defined SLC_FUNC_SIGNATURE_PREFIX
        template<typename Type>
        static consteval auto GetLongName() noexcept
        {
            std::string_view pretty_function{ SLC_FUNC_SIGNATURE };
            auto first = pretty_function.find_first_not_of(' ', pretty_function.find_first_of(SLC_FUNC_SIGNATURE_PREFIX) + 1);
            auto value = pretty_function.substr(first, pretty_function.find_last_of(SLC_FUNC_SIGNATURE_SUFFIX) - first);
            return value;
        }

        template<typename Type>
        static consteval auto GetName() noexcept
        {
            std::string_view long_name = GetLongName<Type>();
            auto first = long_name.find_last_of("::");
            if (first == std::string_view::npos)
                first = long_name.find_last_of(' ');
            else
                first++;
            if (first == std::string_view::npos)
                return long_name;
            return long_name.substr(first, long_name.length() - first);
        }
#endif
    }


    template<typename T>
    struct TypeTraits
    {
#if defined SLC_FUNC_SIGNATURE_PREFIX
        SCONSTEXPR auto LongName = Reflection::GetLongName<T>();
        SCONSTEXPR auto Name = Reflection::GetName<T>();
#endif
        SCONSTEXPR bool IsObject = std::is_class_v<T>;
        SCONSTEXPR bool IsPointer = std::is_pointer_v<T>;
        SCONSTEXPR bool IsEnum = std::is_enum_v<T>;
        SCONSTEXPR bool IsArray = std::is_array_v<T>;
        SCONSTEXPR bool IsConst = std::is_const_v<T>;
        SCONSTEXPR bool IsStandard = std::is_standard_layout_v<T>;

        template<typename R>
        SCONSTEXPR bool IsBaseOf = std::is_base_of_v<T, R>;

        template<typename R>
        SCONSTEXPR bool IsSameAs = std::is_same_v<T, R>;
    };

    template<typename T>
    concept IsEnum = TypeTraits<T>::IsEnum;



    template<typename T>
    struct FunctionTraits;

    template<typename R, typename... Args>
    struct FunctionTraits<std::function<R(Args...)>>
    {
        using ReturnType = R;
        SCONSTEXPR size_t ArgC = sizeof...(Args);

        template <size_t i>
        struct Arg
        {
            using Type = typename std::tuple_element<i, std::tuple<Args...>>::type;
        };
    };

    template <class R, class... Args>
    struct FunctionTraits<R(*)(Args...)>
    {
        using ReturnType = R;
        SCONSTEXPR size_t ArgC = sizeof...(Args);

        template <size_t i>
        struct Arg
        {
            using Type = typename std::tuple_element<i, std::tuple<Args...>>::type;
        };
    };

    template<typename Func, typename TReturn, typename... TArgs>
    concept IsFunc = std::invocable<Func, TArgs...>&&
        requires (Func&& fn, TArgs&&... args) { { fn(std::forward<TArgs>(args)...) } -> std::convertible_to<TReturn>; };



    namespace TypeUtils {

        template<size_t I, typename T, typename TupleType>
        static consteval size_t IndexFunction()
        {
            SASSERT(I < std::tuple_size_v<TupleType>, "The element is not in the tuple");

            using IndexType = typename std::tuple_element<I, TupleType>::type;

            if constexpr (std::is_same_v<T, IndexType>)
                return I;
            else
                return IndexFunction<I + 1, T, TupleType>();
        }
    }

    template<typename... T>
    struct TypeList
    {
        SCONSTEXPR size_t Size = sizeof...(T);

        using TupleType = std::tuple<T...>;
        using VariantType = std::variant<T...>;

        template<typename R>
        SCONSTEXPR bool Contains = std::disjunction<std::is_same<R, T>...>::value;

        template<typename R>
        SCONSTEXPR size_t Index = TypeUtils::IndexFunction<0, R, TupleType>();

        template<size_t I>
        using Type = typename std::tuple_element<I, TupleType>::type;
    };

    template<typename T>
    using Impl = std::unique_ptr<T>;
    template<typename T>
    using Ref = std::shared_ptr<T>;

    template<typename T, typename... Args>
    inline static Impl<T> MakeImpl(Args&&... args) { return std::make_unique<T>(std::forward<Args>(args)...); }
    template<typename T, typename... Args>
    inline static Ref<T> MakeRef(Args&&... args) { return std::make_shared<T>(std::forward<Args>(args)...); }

    template<typename TResult, typename... TArgs>
    using Func = std::function<TResult(TArgs...)>;

    template<typename... TArgs>
    using Action = Func<void, TArgs...>;

    template<typename... T>
    using Predicate = Func<bool, T...>;

    template<std::integral T>
    struct Limits
    {
        SCONSTEXPR T Min     = std::numeric_limits<T>::min();
        SCONSTEXPR T Max     = std::numeric_limits<T>::max();
        SCONSTEXPR T Epsilon = std::numeric_limits<T>::epsilon();
    };
}
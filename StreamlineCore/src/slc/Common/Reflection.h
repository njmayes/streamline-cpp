#pragma once

#include "magic_enum.hpp"

#include <functional>
#include <variant>
#include <string_view>

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

namespace slc {

    namespace detail {

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
        static constexpr auto LongName = detail::GetLongName<T>();
        static constexpr auto Name = detail::GetName<T>();
#endif
        static constexpr bool IsObject = std::is_class_v<T>;
        static constexpr bool IsPointer = std::is_pointer_v<T>;
        static constexpr bool IsEnum = std::is_enum_v<T>;
        static constexpr bool IsArray = std::is_array_v<T>;
        static constexpr bool IsConst = std::is_const_v<T>;
        static constexpr bool IsStandard = std::is_standard_layout_v<T>;

        template<typename R>
        static constexpr bool IsBaseOf = std::is_base_of_v<T, R>;

        template<typename R>
        static constexpr bool IsSameAs = std::is_same_v<T, R>;
    };

    template<typename T>
    concept IsEnum = TypeTraits<T>::IsEnum;

    namespace Enum {

        template<IsEnum T>
        inline static constexpr std::string_view ToString(T enumVal)
        {
            SASSERT(MAGIC_ENUM_SUPPORTED, "Compiler does not support magic enums! Define your own conversions!");

            return magic_enum::enum_name(enumVal);
        }

        template<IsEnum T>
        inline static constexpr T FromString(std::string_view enumStr)
        {
            SASSERT(MAGIC_ENUM_SUPPORTED, "Compiler does not support magic enums! Define your own conversions!");

            auto enumVal = magic_enum::enum_cast<T>(enumStr);
            if (enumVal.has_value())
                return enumVal.value();

            return T();
        }

        template<IsEnum T>
        inline static constexpr bool Contains(std::underlying_type_t<T> value)
        {
            return magic_enum::enum_contains<T>(value);
        }

        template<IsEnum T>
        inline static constexpr size_t Count()
        {
            return magic_enum::enum_count<T>();
        }
    }


    namespace TypeUtils {

        template<size_t I, typename T, typename TupleType>
        static consteval size_t IndexFunction()
        {
            static_assert(I < std::tuple_size_v<TupleType>, "The element is not in the tuple");

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
        static constexpr size_t Size = sizeof...(T);

        using TupleType = std::tuple<T...>;
        using VariantType = std::variant<T...>;

        template<typename R>
        static constexpr bool Contains = std::disjunction<std::is_same<R, T>...>::value;

        template<typename R>
        static constexpr size_t Index = TypeUtils::IndexFunction<0, R, TupleType>();

        template<size_t I>
        using Type = std::tuple_element<I, TupleType>::type;

        template<size_t I>
        using Traits = TypeTraits<typename std::tuple_element<I, TupleType>::type>;
    };

    template<typename T, typename Base>
    concept DerivedFromOnly = std::derived_from<T, Base> and not std::same_as<T, Base>;

    template<typename T>
    concept IsStandard = std::is_standard_layout_v<T>;

    template<typename T>
    concept IsConst = std::is_const_v<T>;

    template<typename From, typename To>
    concept Castable = requires (From from) { static_cast<To>(from); };

    template<typename T>
    concept Integral = std::integral<T>;

    template<typename T>
    concept Numeric = std::is_arithmetic_v<T>;

    template<typename T>
    concept AddAssignable = requires (T&& t)
    {
        t += t;
    };

    template<typename T>
    concept UnaryAddable = requires (T&& t)
    {
        { t + t } -> std::convertible_to<T>;
    };

    template<typename T>
    concept Summable = std::is_default_constructible_v<T> and (AddAssignable<T> or UnaryAddable<T>);

    template<typename T>
    concept UNumeric = std::unsigned_integral<T>;

    template<typename T>
    concept ComparableLess = requires (T&& t1, T&& t2) { std::less<T>(t1, t2); };

    template<typename T>
    concept ComparableGreater = requires (T&& t1, T&& t2) { std::greater<T>(t1, t2); };

    template<typename T>
    concept Sizeable = requires (T&& t)
    {
        { std::size(t) } -> std::convertible_to<size_t>;
    };


    template<typename Func, typename TReturn, typename... TArgs>
    concept IsFunc = std::invocable<Func, TArgs...> and std::convertible_to< std::invoke_result_t< Func, TArgs... >, TReturn >;

    template<typename Func, typename... TArgs>
    concept IsAction = IsFunc<Func, void, TArgs...>;

    template<typename Func, typename... TArgs>
    concept IsPredicate = IsFunc<Func, bool, TArgs...>;

    template<typename T>
    struct FunctionTraits;

    // std::function specialisation
    template<typename R, typename... Args>
    struct FunctionTraits<std::function<R(Args...)>>
    {
        using ReturnType = R;
        static constexpr size_t ArgC = sizeof...(Args);

        template <size_t i>
        struct Arg
        {
            using Type = std::tuple_element<i, std::tuple<Args...>>::type;
        };

        using Arguments = TypeList<Args...>;
    };

    // Function pointer specialisation
    template <typename R, typename... Args>
    struct FunctionTraits<R(*)(Args...)>
    {
        using ReturnType = R;

        static constexpr size_t ArgC = sizeof...(Args);

        template <size_t i>
        struct Arg
        {
            using Type = typename std::tuple_element<i, std::tuple<Args...>>::type;
        };

        using Arguments = TypeList<Args...>;
    };

    // Member function pointer specialisation
    template <typename R, typename O, typename... Args >
    struct FunctionTraits<R(O::*)(Args...)>
    {
        using ObjectType = O;
        using ReturnType = R;
        static constexpr size_t ArgC = sizeof...(Args);

        template <size_t i>
        struct Arg
        {
            using Type = typename std::tuple_element<i, std::tuple<Args...>>::type;
        };

        using Arguments = TypeList<Args...>;
    };


    template<typename T>
    struct PropertyTraits;

    // Member object pointer specialisation
    template <typename P, typename O>
    struct PropertyTraits<P O::*>
    {
        using ObjectType = O;
        using PropType = P;
    };
}
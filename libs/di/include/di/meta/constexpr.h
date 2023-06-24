#pragma once

#include <di/meta/core.h>
#include <di/meta/language.h>
#include <di/meta/operations.h>

namespace di::meta {
namespace detail {
    template<typename T>
    concept ConstexprParam = requires { T::value; } && !concepts::MemberPointer<decltype(&T::value)> &&
                             requires { typename Constexpr<T::value>; };

    template<typename T>
    concept DerivedFromConstexpr = concepts::DerivedFrom<T, Constexpr<T::value>>;

    template<typename T, typename Self>
    concept LhsConstexprParam = ConstexprParam<T> && (concepts::DerivedFrom<T, Self> || !DerivedFromConstexpr<T>);
}

/// @brief A wrapper for a constexpr value.
///
/// @tparam val The value to wrap.
/// @tparam T The type of the value.
///
/// This type defines a value which is lifted into the type-system, and so can be manipulated at compile time. This is
/// especially useful for functions which take compile time values. This type is implicitly convertible to the wrapped
/// value, and so can be used in place of it.
///
/// The helper alias meta::c_ is provided to make using this type more ergonomic, and since the type is able to be
/// deduced, this works a lot better than something like std::integral_constant.
///
/// This type is taken from [p2781](https://wg21.link/p2781).
///
/// @see c_
template<auto val, typename T>
struct Constexpr {
    using Value = T;
    using Type = Constexpr;

    constexpr static auto value = val;

    constexpr operator Value() const { return value; }

    template<detail::ConstexprParam U>
    // NOLINTNEXTLINE(misc-unconventional-assign-operator)
    constexpr auto operator=(U) const -> Constexpr<(val = U::value)> {
        return {};
    }

    template<auto v = val>
    constexpr auto operator+() const -> Constexpr<(+v)> {
        return {};
    }

    template<auto v = val>
    constexpr auto operator-() const -> Constexpr<(-v)> {
        return {};
    }

    template<auto v = val>
    constexpr auto operator~() const -> Constexpr<(~v)> {
        return {};
    }

    template<auto v = val>
    constexpr auto operator!() const -> Constexpr<(!v)> {
        return {};
    }

    template<auto v = val>
    constexpr auto operator&() const -> Constexpr<(&v)> {
        return {};
    }

    template<auto v = val>
    constexpr auto operator*() const -> Constexpr<(*v)> {
        return {};
    }

    template<detail::ConstexprParam... Vs>
    constexpr auto operator()(Vs...) const -> Constexpr<(val(Vs::value...))> {
        return {};
    }

    template<detail::ConstexprParam... Vs>
    constexpr auto operator[](Vs...) const -> Constexpr<(val[Vs::value...])> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator+(U, V) -> Constexpr<(U::value + V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator-(U, V) -> Constexpr<(U::value - V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator*(U, V) -> Constexpr<(U::value * V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator/(U, V) -> Constexpr<(U::value / V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator%(U, V) -> Constexpr<U::value % V::value> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator<<(U, V) -> Constexpr<(U::value << V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator>>(U, V) -> Constexpr<(U::value >> V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator&(U, V) -> Constexpr<(U::value & V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator|(U, V) -> Constexpr<(U::value | V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator^(U, V) -> Constexpr<(U::value ^ V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator&&(U, V) -> Constexpr<(U::value && V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator||(U, V) -> Constexpr<(U::value || V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator==(U, V) -> Constexpr<(U::value == V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator!=(U, V) -> Constexpr<(U::value != V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator<(U, V) -> Constexpr<(U::value < V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator>(U, V) -> Constexpr<(U::value > V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator<=(U, V) -> Constexpr<(U::value <= V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator>=(U, V) -> Constexpr<(U::value >= V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator<=>(U, V) -> Constexpr<(U::value <=> V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator,(U, V) -> Constexpr<(U::value, V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator->*(U, V) -> Constexpr<(U::value->*V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator+=(U, V) -> Constexpr<(U::value += V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator-=(U, V) -> Constexpr<(U::value -= V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator*=(U, V) -> Constexpr<(U::value *= V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator/=(U, V) -> Constexpr<(U::value /= V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    constexpr friend auto operator%=(U, V) -> Constexpr<(U::value %= V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    friend auto operator&=(U, V) -> Constexpr<(U::value &= V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    friend auto operator|=(U, V) -> Constexpr<(U::value |= V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    friend auto operator^=(U, V) -> Constexpr<(U::value ^= V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    friend auto operator<<=(U, V) -> Constexpr<(U::value <<= V::value)> {
        return {};
    }

    template<detail::LhsConstexprParam<Type> U, detail::ConstexprParam V>
    friend auto operator>>=(U, V) -> Constexpr<(U::value >>= V::value)> {
        return {};
    }
};

/// @brief A value of type `Constexpr<val>`.
///
/// @tparam val The value of the `Constexpr`.
///
/// @see Constexpr
template<auto val>
constexpr inline auto c_ = Constexpr<val> {};
}

namespace di::concepts {
namespace detail {
    template<typename T>
    constexpr inline bool constexpr_helper = false;

    template<auto val, typename T>
    constexpr inline bool constexpr_helper<meta::Constexpr<val, T>> = true;
}

/// @brief Checks if `T` is a `Constexpr` instance.
///
/// @tparam T The type to check.
///
/// @see Constexpr
template<typename T>
concept Constexpr = detail::constexpr_helper<T>;

/// @brief Checks if `T` is a `Constexpr` instance with value `U`.
///
/// @tparam T The type to check.
/// @tparam U The value to check.
///
/// @see Constexpr
template<typename T, typename U>
concept ConstexprOf = Constexpr<T> && SameAs<typename T::Value, U>;
}

namespace di {
using meta::c_;
using meta::Constexpr;
}

#pragma once

namespace di::meta {
template<typename T, T constant>
struct IntegralConstant {
    constexpr static T value = constant;
    using Value = T;
    using Type = IntegralConstant;

    constexpr operator T() const { return value; }
    constexpr T operator()() const { return value; }

    constexpr auto operator-() const { return IntegralConstant<T, -constant> {}; }
};
}

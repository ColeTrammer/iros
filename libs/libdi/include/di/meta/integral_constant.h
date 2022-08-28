#pragma once

namespace di::meta {
template<typename T, T constant>
struct IntegralConstant {
    constexpr static T value = constant;
};
}

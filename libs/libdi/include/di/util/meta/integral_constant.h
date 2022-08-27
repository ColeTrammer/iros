#pragma once

namespace di::util::meta {
template<typename T, T constant>
struct IntegralConstant {
    constexpr static T value = constant;
};
}

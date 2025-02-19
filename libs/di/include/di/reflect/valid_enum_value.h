#pragma once

#include "di/reflect/reflect.h"
#include "di/util/bitwise_enum.h"

namespace di::reflection {
namespace detail {
    struct ValidEnumValueFunction {
        template<concepts::ReflectableToEnumerators T>
        requires(!concepts::BitwiseEnum<T>)
        constexpr auto operator()(T value) const {
            auto result = false;
            di::tuple_for_each(
                [&](auto enumerator) {
                    if (enumerator.value == value) {
                        result = true;
                    }
                },
                reflection::reflect(value));
            return result;
        }

        template<concepts::ReflectableToEnumerators T>
        requires(concepts::BitwiseEnum<T>)
        constexpr auto operator()(T value) const {
            auto result = false;
            di::tuple_for_each(
                [&](auto enumerator) {
                    if (enumerator.value == value) {
                        result = true;
                    }
                },
                reflection::reflect(value));
            if (result) {
                return true;
            }

            di::tuple_for_each(
                [&](auto enumerator) {
                    if ((value & enumerator.value) != T(0)) {
                        result = true;
                    }
                    value &= ~enumerator.value;
                },
                reflection::reflect(value));
            return result && value == T(0);
        }
    };
}

constexpr inline auto valid_enum_value = detail::ValidEnumValueFunction {};
}

namespace di {
using reflection::valid_enum_value;
}

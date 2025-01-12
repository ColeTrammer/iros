#pragma once

#include "di/reflect/reflect.h"

namespace di::reflection {
namespace detail {
    struct ValidEnumValueFunction {
        constexpr auto operator()(concepts::ReflectableToEnumerators auto value) const {
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
    };
}

constexpr inline auto valid_enum_value = detail::ValidEnumValueFunction {};
}

namespace di {
using reflection::valid_enum_value;
}

#pragma once

#include "di/container/string/string_view.h"
#include "di/reflect/reflect.h"

namespace di::reflection {
namespace detail {
    struct EnumToStringFunction {
        constexpr auto operator()(concepts::ReflectableToEnumerators auto value) const {
            auto result = "[<Invalid Enum Value>]"_sv;
            di::tuple_for_each(
                [&](auto enumerator) {
                    if (enumerator.value == value) {
                        // NOTE: the strings in this library are compile-time values (with fixed length), so we need to
                        // convert them to a normal string view.
                        result = di::container::fixed_string_to_utf8_string_view<enumerator.name>();
                    }
                },
                reflection::reflect(value));
            return result;
        }
    };
}

constexpr inline auto enum_to_string = detail::EnumToStringFunction {};
}

namespace di {
using reflection::enum_to_string;
}

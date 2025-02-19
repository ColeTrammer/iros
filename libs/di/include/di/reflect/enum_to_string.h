#pragma once

#include "di/container/string/string.h"
#include "di/container/string/string_view.h"
#include "di/reflect/reflect.h"
#include "di/util/bitwise_enum.h"

namespace di::reflection {
namespace detail {
    struct EnumToStringFunction {
        template<concepts::ReflectableToEnumerators T>
        requires(!concepts::BitwiseEnum<T>)
        constexpr auto operator()(T value) const {
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

        template<concepts::ReflectableToEnumerators T>
        requires(concepts::BitwiseEnum<T>)
        constexpr auto operator()(T value) const {
            auto result = ""_s;
            auto matched = false;
            // First check for exact matches.
            di::tuple_for_each(
                [&](auto enumerator) {
                    if (enumerator.value == value) {
                        result = di::container::fixed_string_to_utf8_string_view<enumerator.name>().to_owned();
                        matched = true;
                        value = T(0);
                    }
                },
                reflection::reflect(value));

            // Now, if we didn't match exactly, try each enum 1 by 1 in reverse. By iterating
            // in reverse we ensure composite enums are matched first, if the user declared things
            // correctly.
            if (value != T(0)) {
                di::tuple_for_each_reverse(
                    [&](auto enumerator) {
                        if ((enumerator.value & value) != T(0)) {
                            if (!result.empty()) {
                                result.push_back('|');
                            }
                            result += di::container::fixed_string_to_utf8_string_view<enumerator.name>() | reverse;
                            matched = true;
                            value &= ~enumerator.value;
                        }
                    },
                    reflection::reflect(value));
                auto new_result = reverse(result) | di::to<String>();
                result = di::move(new_result);
            }

            // If we matched nothing or have extraneous extra values, the enum is invalid.
            if (!matched || value != T(0)) {
                result = "[<Invalid Enum Value>]"_s;
            }
            return result;
        }
    };
}

constexpr inline auto enum_to_string = detail::EnumToStringFunction {};
}

namespace di {
using reflection::enum_to_string;
}

#pragma once

#include "di/container/string/constant_string.h"

namespace di::container::string {
constexpr auto iterator_at_offset(concepts::detail::ConstantString auto const& string, size_t index) {
    return lift_bool(encoding::valid_byte_offset(string.encoding(), string.span(), index)) % [&] {
        return encoding::make_iterator(string.encoding(), string.span(), index);
    };
}
}

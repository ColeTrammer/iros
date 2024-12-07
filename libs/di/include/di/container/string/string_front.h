#pragma once

#include "di/container/string/constant_string.h"
#include "di/container/string/string_begin.h"
#include "di/container/string/string_empty.h"

namespace di::container::string {
constexpr auto front(concepts::detail::ConstantString auto const& string) {
    return lift_bool(!string::empty(string)) % [&] {
        return *container::begin(string);
    };
}
}

#pragma once

#include "di/container/string/constant_string.h"
#include "di/container/string/string_empty.h"
#include "di/container/string/string_end.h"

namespace di::container::string {
constexpr auto back(concepts::detail::ConstantString auto const& string) {
    return lift_bool(!string::empty(string)) % [&] {
        return *container::prev(container::end(string));
    };
}
}

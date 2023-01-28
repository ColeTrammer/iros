#pragma once

#include <di/container/string/constant_string.h>

namespace di::container::string {
constexpr size_t empty(concepts::detail::ConstantString auto const& string) {
    return string.span().empty();
}
}

#pragma once

#include <di/container/string/constant_string.h>

namespace di::container::string {
constexpr size_t size_code_units(concepts::detail::ConstantString auto const& string) {
    return string.span().size();
}
}

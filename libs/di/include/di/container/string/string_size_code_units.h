#pragma once

#include <di/container/string/constant_string.h>

namespace di::container::string {
constexpr auto size_code_units(concepts::detail::ConstantString auto const& string) -> size_t {
    return string.span().size();
}
}

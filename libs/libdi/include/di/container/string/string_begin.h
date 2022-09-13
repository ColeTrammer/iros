#pragma once

#include <di/container/string/constant_string.h>

namespace di::container::string {
constexpr auto begin(concepts::detail::ConstantString auto const& string) {
    auto [begin, end] = encoding::code_point_view(string.encoding(), string.span());
    (void) end;
    return begin;
}
}

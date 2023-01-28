#pragma once

#include <di/container/string/constant_string.h>

namespace di::container::string {
constexpr auto unicode_code_points(concepts::detail::ConstantString auto const& string) {
    return encoding::unicode_code_point_view(string.encoding(), string.span());
}
}

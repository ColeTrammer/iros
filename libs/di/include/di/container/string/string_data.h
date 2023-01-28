#pragma once

#include <di/container/string/constant_string.h>

namespace di::container::string {
constexpr auto data(concepts::detail::ConstantString auto const& string) {
    return string.span().data();
}
}

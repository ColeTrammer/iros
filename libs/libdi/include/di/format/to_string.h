#pragma once

#include <di/container/string/string.h>

namespace di::format {
template<typename T>
constexpr auto to_string(T&&) {
    return container::String {};
}
}

#pragma once

namespace di::util {
template<typename T>
constexpr auto as_const_pointer(T const* pointer) {
    return pointer;
}
}
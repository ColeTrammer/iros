#pragma once

namespace di::util {
template<typename T>
constexpr auto as_const_pointer(T const* pointer) {
    return pointer;
}
}

namespace di {
using util::as_const_pointer;
}

#pragma once

namespace di::util {
template<typename T>
constexpr void destroy_at(T* pointer) {
    pointer->~T();
}
}

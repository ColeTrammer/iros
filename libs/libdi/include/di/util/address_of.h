#pragma once

namespace di::util {
// This uses a compiler builtin because user-defined types can overload operator&.
template<typename T>
constexpr T* address_of(T& value) {
    return __builtin_addressof(value);
}

// Disallow getting the address of a temporary.
template<class T>
void address_of(const T&&) = delete;
}

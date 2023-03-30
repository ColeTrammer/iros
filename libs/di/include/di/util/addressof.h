#pragma once

#ifdef DI_USE_STD
#include <utility>
#else
namespace std {
// This uses a compiler builtin because user-defined types can overload operator&.
template<typename T>
constexpr T* addressof(T& value) {
    return __builtin_addressof(value);
}

// Disallow getting the address of a temporary.
template<class T>
void addressof(T const&&) = delete;
}
#endif

namespace di::util {
using std::addressof;
}

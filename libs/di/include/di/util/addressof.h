#pragma once

#ifndef DI_NO_USE_STD
#include <memory>
#else
namespace std {
// This uses a compiler builtin because user-defined types can overload operator&.
template<typename T>
constexpr auto addressof(T& value) -> T* {
    return __builtin_addressof(value);
}

// Disallow getting the address of a temporary.
template<class T>
auto addressof(T const&&) -> T* = delete;
}
#endif

namespace di::util {
using std::addressof;
}

namespace di {
using util::addressof;
}

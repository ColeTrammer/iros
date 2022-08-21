#pragma once

#include <di/util/meta/remove_reference.h>

#ifdef DI_USE_STD
#include <utilities>
#else
namespace std {
template<typename T>
constexpr T&& forward(di::util::meta::RemoveReference<T>& value) noexcept {
    return static_cast<T&&>(value);
}

template<typename T>
constexpr T&& forward(di::util::meta::RemoveReference<T>&& value) noexcept {
    return static_cast<T&&>(value);
}
}
#endif

namespace di::util {
using std::forward;
}

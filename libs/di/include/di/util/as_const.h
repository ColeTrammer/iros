#pragma once

#include <di/meta/add_const.h>

#ifndef DI_NO_USE_STD
#include <utility>
#else
namespace std {
template<typename T>
constexpr di::meta::AddConst<T>& as_const(T& value) noexcept {
    return value;
}

template<typename T>
constexpr void as_const(T const&&) = delete;
}
#endif

namespace di::util {
using std::as_const;
}

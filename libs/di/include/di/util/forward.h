#pragma once

#include "di/meta/core.h"

#ifndef DI_NO_USE_STD
#include <utility>
#else
namespace std {
template<typename T>
constexpr auto forward(di::meta::RemoveReference<T>& value) noexcept -> T&& {
    return static_cast<T&&>(value);
}

template<typename T>
constexpr auto forward(di::meta::RemoveReference<T>&& value) noexcept -> T&& {
    return static_cast<T&&>(value);
}
}
#endif

namespace di::util {
using std::forward;
}

namespace di {
using util::forward;
}

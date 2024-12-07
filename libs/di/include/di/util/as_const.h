#pragma once

#include "di/meta/util.h"

#ifndef DI_NO_USE_STD
#include <utility>
#else
namespace std {
template<typename T>
constexpr auto as_const(T& value) noexcept -> di::meta::AddConst<T>& {
    return value;
}

template<typename T>
constexpr auto as_const(T const&&) -> di::meta::AddConst<T>& = delete;
}
#endif

namespace di::util {
using std::as_const;
}

namespace di {
using util::as_const;
}

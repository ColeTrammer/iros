#pragma once

#include <di/util/meta/add_const.h>

#ifdef DI_USE_STD
#include <utility>
#else
namespace std {
template<typename T>
constexpr di::util::meta::AddConst<T>& move(T& value) noexcept {
    return value;
}

template<typename T>
constexpr void as_const(T const&&) = delete;
}
#endif

namespace di::util {
using std::as_const;
}

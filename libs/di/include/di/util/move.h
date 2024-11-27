#pragma once

#include <di/meta/core.h>

#ifndef DI_NO_USE_STD
#include <utility>
#else
namespace std {
template<typename T>
constexpr auto move(T&& value) noexcept -> di::meta::RemoveReference<T>&& {
    return static_cast<di::meta::RemoveReference<T>&&>(value);
}
}
#endif

namespace di::util {
using std::move;
}

namespace di {
using util::move;
}

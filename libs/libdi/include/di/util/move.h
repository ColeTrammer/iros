#pragma once

#include <di/meta/remove_reference.h>

#ifdef DI_USE_STD
#include <utility>
#else
namespace std {
template<typename T>
constexpr di::util::RemoveReference<T>&& move(T&& value) noexcept {
    return static_cast<di::meta::RemoveReference<T>&&>(value);
}
}
#endif

namespace di::util {
using std::move;
}

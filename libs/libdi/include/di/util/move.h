#pragma once

#include <di/util/meta/remove_reference.h>

#ifdef DI_USE_STD
#include <utility>
#else
namespace std {
template<typename T>
constexpr di::util::meta::RemoveReference<T>&& move(T&& value) noexcept {
    return static_cast<di::util::meta::RemoveReference<T>&&>(value);
}
}
#endif

namespace di::util {
using std::move;
}

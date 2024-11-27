#pragma once

#include <di/util/forward.h>
#include <di/util/get_in_place.h>

namespace di::util {
template<types::size_t index, typename T>
constexpr auto get(T&& value) -> decltype(auto) {
    return util::get_in_place(c_<index>, util::forward<T>(value));
}

template<typename Type, typename T>
constexpr auto get(T&& value) -> decltype(auto) {
    return util::get_in_place(types::in_place_type<Type>, util::forward<T>(value));
}
}

namespace di {
using util::get;
}

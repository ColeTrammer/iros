#pragma once

#include <di/util/forward.h>
#include <di/util/get_in_place.h>

namespace di::util {
template<types::size_t index, typename T>
requires(requires(T&& value) { util::get_in_place(types::in_place_index<index>, util::forward<T>(value)); })
constexpr decltype(auto) get(T&& value) {
    return util::get_in_place(types::in_place_index<index>, util::forward<T>(value));
}

template<typename Type, typename T>
requires(requires(T&& value) { util::get_in_place(types::in_place_type<Type>, util::forward<T>(value)); })
constexpr decltype(auto) get(T&& value) {
    return util::get_in_place(types::in_place_type<Type>, util::forward<T>(value));
}
}

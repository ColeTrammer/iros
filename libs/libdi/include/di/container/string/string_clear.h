#pragma once

#include <di/container/string/mutable_string.h>
#include <di/container/vector/vector_append_container.h>

namespace di::container::string {
template<concepts::detail::MutableString Str>
constexpr void clear(Str& string) {
    vector::clear(string);
}
}

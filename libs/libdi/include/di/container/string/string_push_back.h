#pragma once

#include <di/container/string/mutable_string.h>
#include <di/container/vector/vector_append_container.h>

namespace di::container::string {
template<concepts::detail::MutableString Str, typename Enc = meta::Encoding<Str>, typename P = meta::EncodingCodePoint<Enc>>
constexpr void push_back(Str& string, P code_point) {
    vector::append_container(string, encoding::convert_to_code_units(string.encoding(), code_point));
}
}

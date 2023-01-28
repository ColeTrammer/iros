#pragma once

#include <di/container/string/mutable_string.h>
#include <di/container/vector/vector_append_container.h>
#include <di/function/into_void.h>

namespace di::container::string {
template<concepts::detail::MutableString Str, typename Enc = meta::Encoding<Str>,
         typename P = meta::EncodingCodePoint<Enc>>
constexpr auto push_back(Str& string, P code_point) {
    if constexpr (encoding::NullTerminated<Enc>) {
        return invoke_as_fallible([&] {
                   return vector::append_container(string,
                                                   encoding::convert_to_code_units(string.encoding(), code_point));
               }) >>
                   [&] {
                       return as_fallible(vector::emplace_back(string)) % [&](auto) {
                           vector::pop_back(string);
                       };
                   } |
               try_infallible;
    } else {
        return vector::append_container(string, encoding::convert_to_code_units(string.encoding(), code_point));
    }
}
}

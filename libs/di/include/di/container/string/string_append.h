#pragma once

#include <di/container/string/mutable_string.h>
#include <di/container/vector/vector_append_container.h>
#include <di/container/vector/vector_pop_back.h>
#include <di/container/view/join.h>
#include <di/container/view/transform.h>
#include <di/function/bind_front.h>

namespace di::container::string {
template<concepts::detail::MutableString Str, typename Enc = meta::Encoding<Str>,
         typename P = meta::EncodingCodePoint<Enc>, concepts::ContainerCompatible<P> Con>
constexpr auto append(Str& string, Con&& container) {
    if constexpr (encoding::NullTerminated<Enc>) {
        return invoke_as_fallible([&] {
                   return vector::append_container(
                       string,
                       util::forward<Con>(container) |
                           view::transform(function::bind_front(encoding::convert_to_code_units, string.encoding())) |
                           view::join);
               }) >>
                   [&] {
                       return as_fallible(vector::emplace_back(string)) % [&](auto&) {
                           vector::pop_back(string);
                       };
                   } |
               try_infallible;
    } else {
        return vector::append_container(
            string, util::forward<Con>(container) |
                        view::transform(function::bind_front(encoding::convert_to_code_units, string.encoding())) |
                        view::join);
    }
}
}

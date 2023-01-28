#pragma once

#include <di/container/algorithm/find_first_not_of.h>
#include <di/container/string/constant_string.h>
#include <di/container/string/string_begin.h>
#include <di/container/string/string_end.h>
#include <di/container/view/single.h>

namespace di::container::string {
template<concepts::detail::ConstantString Str, typename Enc = meta::Encoding<Str>>
constexpr auto find_first_not_of(Str const& string, meta::EncodingCodePoint<Str> code_point) {
    return container::find_first_not_of(View(string::begin(string), string::end(string)), view::single(code_point));
}

template<concepts::detail::ConstantString Str, typename Enc = meta::Encoding<Str>,
         concepts::ContainerCompatible<meta::EncodingCodePoint<Enc>> Con>
requires(concepts::SameAs<Enc, meta::Encoding<Con>>)
constexpr auto find_first_not_of(Str const& string, Con&& container) {
    return container::find_first_not_of(View(string::begin(string), string::end(string)),
                                        util::forward<Con>(container));
}
}

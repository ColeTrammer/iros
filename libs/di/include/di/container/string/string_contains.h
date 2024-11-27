#pragma once

#include <di/container/algorithm/contains.h>
#include <di/container/algorithm/contains_subrange.h>
#include <di/container/string/constant_string.h>
#include <di/container/string/string_begin.h>
#include <di/container/string/string_end.h>
#include <di/container/view/single.h>

namespace di::container::string {
template<concepts::detail::ConstantString Str, typename Enc = meta::Encoding<Str>>
constexpr auto contains(Str const& string, meta::EncodingCodePoint<Str> code_point) -> bool {
    return container::contains(View(string::begin(string), string::end(string)), code_point);
}

template<concepts::detail::ConstantString Str, typename Enc = meta::Encoding<Str>,
         concepts::ContainerCompatible<meta::EncodingCodePoint<Enc>> Con>
requires(concepts::SameAs<Enc, meta::Encoding<Con>>)
constexpr auto contains(Str const& string, Con&& container) -> bool {
    return container::contains_subrange(View(string::begin(string), string::end(string)),
                                        util::forward<Con>(container));
}
}

#pragma once

#include "di/container/string/constant_string.h"
#include "di/container/string/string_end.h"
#include "di/container/string/string_view_impl_forward_declaration.h"

namespace di::container::string {
template<concepts::detail::ConstantString Str, typename Enc = meta::Encoding<Str>>
constexpr auto substr(Str const& string, meta::EncodingIterator<Enc> first,
                      Optional<meta::EncodingIterator<Enc>> last = {}) {
    return StringViewImpl<Enc> { first, last.value_or(string::end(string)), string.encoding() };
}
}

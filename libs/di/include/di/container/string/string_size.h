#pragma once

#include <di/container/string/constant_string.h>
#include <di/container/string/encoding.h>

namespace di::container::string {
template<concepts::detail::ConstantString Str, typename Enc = meta::Encoding<Str>>
requires(encoding::Contiguous<Enc>)
constexpr size_t size(Str const& string) {
    return string.span().size();
}
}

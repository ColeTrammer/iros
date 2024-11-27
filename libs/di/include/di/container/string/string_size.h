#pragma once

#include <di/container/string/constant_string.h>
#include <di/container/string/encoding.h>

namespace di::container::string {
template<concepts::detail::ConstantString Str, typename Enc = meta::Encoding<Str>>
requires(encoding::Contiguous<Enc>)
constexpr auto size(Str const& string) -> size_t {
    return string.span().size();
}
}

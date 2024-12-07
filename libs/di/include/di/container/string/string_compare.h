#pragma once

#include "di/container/algorithm/compare.h"
#include "di/container/string/constant_string.h"
#include "di/container/string/encoding.h"
#include "di/meta/core.h"

namespace di::container::string {
template<concepts::detail::ConstantString S, concepts::detail::ConstantString T>
requires(concepts::SameAs<meta::Encoding<S>, meta::Encoding<T>>)
constexpr auto compare(S const& s, T const& t) {
    return container::compare(encoding::code_point_view(s.encoding(), s.span()),
                              encoding::code_point_view(t.encoding(), t.span()));
}
}

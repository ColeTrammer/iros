#pragma once

#include <di/container/string/encoding.h>
#include <di/meta/operations.h>

namespace di::concepts::detail {
template<typename T>
concept ConstantString = HasEncoding<T> && requires(T const& string) {
    { string.encoding() } -> ConvertibleTo<meta::Encoding<T>>;
    { string.span() } -> ConvertibleTo<vocab::Span<meta::EncodingCodeUnit<meta::Encoding<T>> const>>;
};
}

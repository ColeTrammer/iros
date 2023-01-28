#pragma once

#include <di/container/string/constant_string.h>
#include <di/container/vector/mutable_vector.h>

namespace di::concepts::detail {
template<typename T>
concept MutableString = ConstantString<T> && MutableVector<T> &&
                        SameAs<meta::EncodingCodeUnit<meta::Encoding<T>>, meta::detail::VectorValue<T>>;
}

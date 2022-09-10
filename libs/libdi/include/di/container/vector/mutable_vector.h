#pragma once

#include <di/concepts/language_void.h>
#include <di/concepts/maybe_fallible.h>
#include <di/concepts/same_as.h>
#include <di/container/vector/constant_vector.h>
#include <di/types/prelude.h>

namespace di::concepts::detail {
template<typename T>
concept MutableVector = ConstantVector<T> && requires(T& value, T const& cvalue, size_t n) {
                                                 typename T::Value;
                                                 { cvalue.capacity() } -> SameAs<size_t>;
                                                 { cvalue.max_size() } -> SameAs<size_t>;
                                                 { value.reserve(n) } -> MaybeFallible<void>;
                                                 { value.shrink_to_fit() } -> LanguageVoid;
                                                 { value.assume_size(n) } -> LanguageVoid;
                                             };
}

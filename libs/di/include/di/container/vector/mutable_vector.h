#pragma once

#include <di/concepts/default_constructible.h>
#include <di/concepts/language_void.h>
#include <di/concepts/maybe_fallible.h>
#include <di/concepts/move_constructible.h>
#include <di/concepts/same_as.h>
#include <di/container/concepts/contiguous_container.h>
#include <di/container/vector/constant_vector.h>
#include <di/types/prelude.h>

namespace di::concepts::detail {
template<typename T>
concept MutableVector = ConstantVector<T> && DefaultConstructible<T> && MoveConstructible<T> &&
                        requires(T& value, T const& cvalue, size_t n) {
                            { cvalue.capacity() } -> SameAs<size_t>;
                            { cvalue.max_size() } -> SameAs<size_t>;
                            { value.reserve_from_nothing(n) } -> MaybeFallible<void>;
                            { value.assume_size(n) } -> LanguageVoid;
                        };
}

namespace di::meta::detail {
template<concepts::detail::MutableVector T>
using VectorAllocResult = decltype(util::declval<T&>().reserve_from_nothing(util::declval<size_t>()));
}

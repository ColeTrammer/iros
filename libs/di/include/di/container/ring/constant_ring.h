#pragma once

#include <di/concepts/const.h>
#include <di/container/vector/constant_vector.h>
#include <di/meta/conditional.h>
#include <di/types/prelude.h>

namespace di::concepts::detail {
template<typename T>
concept ConstantRing = ConstantVector<T> && requires(T const& clvalue) {
    { clvalue.head() } -> SameAs<usize>;
    { clvalue.tail() } -> SameAs<usize>;
    { clvalue.capacity() } -> SameAs<usize>;
};
}

namespace di::meta::detail {
template<concepts::detail::ConstantRing T>
using RingValue = meta::Conditional<concepts::Const<T>, VectorConstValue<T>, VectorValue<T>>;
}

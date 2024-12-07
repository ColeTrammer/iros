#pragma once

#include "di/bit/bitset/prelude.h"
#include "di/meta/core.h"
#include "di/meta/operations.h"
#include "di/types/prelude.h"
#include "di/util/declval.h"

namespace di::meta {
template<typename T>
requires(requires { typename T::Value; })
using BitValue = T::Value;
}

namespace di::concepts {
template<typename T>
concept BitTag =
    requires { typename meta::BitValue<T>; } && requires(meta::BitValue<T> value, BitSet<0> bitset, T const tag) {
        T::value_into_bits(bitset, value);
        { T::bits_into_value(bitset) } -> SameAs<meta::BitValue<T>>;
        { tag.get() } -> SameAs<meta::BitValue<T>>;
    } && ConstructibleFrom<T, meta::BitValue<T>>;
}

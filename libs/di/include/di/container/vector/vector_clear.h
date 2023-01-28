#pragma once

#include <di/concepts/trivially_destructible.h>
#include <di/container/vector/mutable_vector.h>
#include <di/container/view/view.h>
#include <di/util/address_of.h>
#include <di/util/destroy.h>

namespace di::container::vector {
template<concepts::detail::MutableVector Vec>
constexpr void clear(Vec& vector) {
    if constexpr (concepts::TriviallyDestructible<meta::detail::VectorValue<Vec>>) {
        (void) vector;
    } else {
        util::destroy(vector.begin(), vector.end());
    }
    vector.assume_size(0);
}
}

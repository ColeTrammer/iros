#pragma once

#include <di/container/algorithm/destroy.h>
#include <di/container/vector/mutable_vector.h>
#include <di/container/vector/vector_begin.h>
#include <di/container/vector/vector_end.h>
#include <di/container/view/view.h>
#include <di/util/addressof.h>

namespace di::container::vector {
template<concepts::detail::MutableVector Vec>
constexpr void clear(Vec& vector) {
    container::destroy(vector::begin(vector), vector::end(vector));
    vector.assume_size(0);
}
}

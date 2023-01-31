#pragma once

#include <di/container/algorithm/uninitialized_relocate.h>
#include <di/container/vector/mutable_vector.h>
#include <di/container/vector/vector_data.h>
#include <di/container/vector/vector_size.h>
#include <di/types/prelude.h>
#include <di/util/create.h>
#include <di/util/swap.h>
#include <di/vocab/expected/prelude.h>

namespace di::container::vector {
template<concepts::detail::MutableVector Vec, typename R = meta::detail::VectorAllocResult<Vec>>
constexpr R reserve(Vec& vector, size_t capacity) {
    if (capacity <= vector.capacity()) {
        return util::create<R>();
    }

    auto size = vector::size(vector);
    auto temp = Vec();
    return invoke_as_fallible([&] {
               return temp.reserve_from_nothing(capacity);
           }) % [&] {
        auto new_buffer = vector::data(temp);
        container::uninitialized_relocate(vector::begin(vector), vector::end(vector), new_buffer,
                                          new_buffer + capacity);
        temp.assume_size(size);
        vector.assume_size(0);
        util::swap(vector, temp);
    } | try_infallible;
}
}

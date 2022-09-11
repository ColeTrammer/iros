#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/copy_constructible.h>
#include <di/concepts/move_constructible.h>
#include <di/concepts/remove_cvref_same_as.h>
#include <di/container/vector/mutable_vector.h>
#include <di/container/vector/vector_data.h>
#include <di/container/vector/vector_reserve.h>
#include <di/container/vector/vector_size.h>
#include <di/util/construct_at.h>
#include <di/util/forward.h>
#include <di/util/swap.h>
#include <di/util/uninitialized_relocate.h>
#include <di/util/uninitialized_relocate_backwards.h>
#include <di/vocab/expected/prelude.h>

namespace di::container::vector {
template<typename Vec = concepts::detail::MutableVector, typename... Args, typename CIter = meta::detail::VectorConstIterator<Vec>>
requires(concepts::ConstructibleFrom<meta::detail::VectorValue<Vec>, Args...>)
constexpr auto emplace(Vec& vector, CIter position, Args&&... args) {
    auto size = vector::size(vector);
    auto new_size = size + 1;
    auto end = vector::end(vector);

    if (size >= vector.capacity()) {
        auto new_vector = Vec();
        new_vector.reserve_from_nothing(size + 1);

        auto new_data = vector::data(new_vector);
        auto new_data_end = new_data + new_size;
        auto [next_in, next_out] = util::uninitialized_relocate(vector::begin(vector), position, new_data, new_data_end);
        util::uninitialized_relocate(next_in, end, next_out + 1, new_data_end);
        util::construct_at(next_out, util::forward<Args>(args)...);
        new_vector.assume_size(0);
        util::swap(vector, new_vector);
        return next_in;
    }

    util::uninitialized_relocate_backwards(position, end, position + 1, end + 1);
    auto result = util::construct_at(vector::iterator(vector, position), util::forward<Args>(args)...);
    vector.assume_size(new_size);
    return result;
}
}

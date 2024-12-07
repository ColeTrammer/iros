#pragma once

#include "di/container/vector/mutable_vector.h"
#include "di/container/vector/vector_data.h"
#include "di/container/vector/vector_reserve.h"
#include "di/container/vector/vector_size.h"
#include "di/meta/operations.h"
#include "di/meta/util.h"
#include "di/util/construct_at.h"
#include "di/util/forward.h"
#include "di/util/reference_wrapper.h"
#include "di/vocab/expected/prelude.h"

namespace di::container::vector {
template<concepts::detail::MutableVector Vec, typename... Args>
requires(concepts::ConstructibleFrom<meta::detail::VectorValue<Vec>, Args...>)
constexpr auto emplace_back(Vec& vector, Args&&... args) -> decltype(auto) {
    auto size = vector::size(vector);
    return invoke_as_fallible([&] {
               return vector::reserve(vector, size + 1);
           }) % [&] {
        auto end = vector::data(vector) + size;
        auto result = util::construct_at(end, util::forward<Args>(args)...);
        vector.assume_size(size + 1);
        return util::ref(*result);
    } | try_infallible;
}
}

#pragma once

#include "di/container/action/prelude.h"
#include "di/container/algorithm/destroy.h"
#include "di/container/vector/mutable_vector.h"
#include "di/container/vector/vector_emplace_back.h"
#include "di/container/vector/vector_end.h"
#include "di/container/vector/vector_size.h"
#include "di/container/view/prelude.h"
#include "di/meta/operations.h"
#include "di/types/prelude.h"
#include "di/util/create.h"
#include "di/vocab/expected/prelude.h"

namespace di::container::vector {
template<concepts::detail::MutableVector Vec, typename T = meta::detail::VectorValue<Vec>,
         typename R = meta::detail::VectorAllocResult<Vec>>
requires(concepts::DefaultConstructible<T>)
constexpr auto resize(Vec& vector, size_t count) -> R {
    auto size = vector::size(vector);
    if (count == size) {
        return util::create<R>();
    }

    if (count < size) {
        auto end = vector::end(vector);
        container::destroy(end - count, end);
        vector.assume_size(count);
        return util::create<R>();
    }

    return container::sequence(range(count - size), [&](auto) {
        return as_fallible(vector::emplace_back(vector));
    });
}

template<concepts::detail::MutableVector Vec, typename T = meta::detail::VectorValue<Vec>,
         typename R = meta::detail::VectorAllocResult<Vec>>
requires(concepts::CopyConstructible<T>)
constexpr auto resize(Vec& vector, size_t count, T const& value) -> R {
    auto size = vector::size(vector);
    if (count < size) {
        auto end = vector::end(vector);
        container::destroy(end - count, end);
        vector.assume_size(count);
        return util::create<R>();
    }
    if (count > size) {
        return container::sequence(range(count - size), [&](auto) {
            return as_fallible(vector::emplace_back(vector, value));
        });
    }
    return util::create<R>();
}
}

#pragma once

#include <di/concepts/copy_constructible.h>
#include <di/concepts/default_constructible.h>
#include <di/container/action/prelude.h>
#include <di/container/vector/mutable_vector.h>
#include <di/container/vector/vector_emplace_back.h>
#include <di/container/vector/vector_end.h>
#include <di/container/vector/vector_size.h>
#include <di/container/view/prelude.h>
#include <di/types/prelude.h>
#include <di/util/create.h>
#include <di/util/destroy.h>
#include <di/vocab/expected/prelude.h>

namespace di::container::vector {
template<concepts::detail::MutableVector Vec, typename T = meta::detail::VectorValue<Vec>,
         typename R = meta::detail::VectorAllocResult<Vec>>
requires(concepts::DefaultConstructible<T>)
constexpr R resize(Vec& vector, size_t count) {
    auto size = vector::size(vector);
    if (count == size) {
        return util::create<R>();
    }

    if (count < size) {
        auto end = vector::end(vector);
        util::destroy(end - count, end);
        vector.assume_size(count);
        return util::create<R>();
    }

    return container::sequence(range(count - size), [&](auto) {
        return as_fallible(vector::emplace_back(vector));
    });
}

template<concepts::detail::MutableVector Vec, typename T = meta::detail::VectorValue<Vec>>
requires(concepts::CopyConstructible<T>)
constexpr void resize(Vec& vector, size_t count, T const& value) {
    auto size = vector::size(vector);
    if (count < size) {
        auto end = vector::end(vector);
        util::destroy(end - count, end);
        vector.assume_size(count);
    } else if (count > size) {
        for (size_t i = 0; i < count - size; i++) {
            vector::emplace_back(vector, value);
        }
    }
}
}

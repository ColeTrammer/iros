#pragma once

#include <di/container/action/sequence.h>
#include <di/container/concepts/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/container/vector/mutable_vector.h>
#include <di/container/vector/vector_emplace_back.h>
#include <di/types/prelude.h>
#include <di/util/move.h>
#include <di/vocab/expected/prelude.h>

namespace di::container::vector {
template<concepts::detail::MutableVector Vec, concepts::InputContainer Con, typename T = meta::detail::VectorValue<Vec>,
         typename R = meta::detail::VectorAllocResult<Vec>>
requires(concepts::ContainerCompatible<Con, T>)
constexpr R append_container(Vec& vector, Con&& container) {
    return container::sequence(util::forward<Con>(container), [&]<typename X>(X&& value) {
        return as_fallible(vector::emplace_back(vector, util::forward<X>(value)));
    });
}
}

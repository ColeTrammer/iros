#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/container/vector/mutable_vector.h>
#include <di/container/vector/vector_emplace_back.h>
#include <di/types/prelude.h>

namespace di::container::vector {
template<concepts::detail::MutableVector Vec, concepts::InputContainer Con, typename T = meta::detail::VectorValue<Vec>>
requires(concepts::ContainerCompatible<Con, T>)
constexpr void append_container(Vec& vector, Con&& container) {
    for (auto&& x : container) {
        vector::emplace_back(vector, static_cast<decltype(x)&&>(x));
    }
}
}

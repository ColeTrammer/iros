#pragma once

#include "di/container/action/sequence.h"
#include "di/container/concepts/prelude.h"
#include "di/container/meta/prelude.h"
#include "di/container/vector/constant_vector.h"
#include "di/container/vector/mutable_vector.h"
#include "di/container/vector/vector_emplace.h"
#include "di/container/vector/vector_emplace_back.h"
#include "di/container/view/view.h"
#include "di/meta/vocab.h"
#include "di/types/prelude.h"
#include "di/util/move.h"
#include "di/vocab/expected/invoke_as_fallible.h"
#include "di/vocab/expected/prelude.h"

namespace di::container::vector {
template<concepts::detail::MutableVector Vec, concepts::InputContainer Con, typename T = meta::detail::VectorValue<Vec>,
         typename R = meta::detail::VectorAllocResult<Vec>>
requires(concepts::ContainerCompatible<Con, T>)
constexpr auto append_container(Vec& vector, Con&& container) -> R {
    return container::sequence(util::forward<Con>(container), [&]<typename X>(X&& value) {
        return as_fallible(vector::emplace_back(vector, util::forward<X>(value)));
    });
}

template<concepts::detail::MutableVector Vec, concepts::InputContainer Con, typename T = meta::detail::VectorValue<Vec>,
         typename It = meta::detail::VectorIterator<Vec>, typename Cit = meta::detail::VectorConstIterator<Vec>,
         typename R = meta::LikeExpected<meta::detail::VectorAllocResult<Vec>, View<It, It>>>
requires(concepts::ContainerCompatible<Con, T>)
constexpr auto insert_container(Vec& vector, Cit it, Con&& container) -> R {
    auto start_offset = it - vector::begin(vector);
    return invoke_as_fallible([&] {
               return container::sequence(util::forward<Con>(container), [&]<typename X>(X&& value) {
                   return as_fallible(vector::emplace(vector, it++, util::forward<X>(value)));
               });
           }) % [&] {
        auto end_offset = it - vector::begin(vector);
        return View {
            vector::begin(vector) + start_offset,
            vector::begin(vector) + end_offset,
        };
    } | try_infallible;
}
}

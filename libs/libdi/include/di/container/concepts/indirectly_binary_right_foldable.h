#pragma once

#include <di/container/concepts/indirectly_binary_left_foldable.h>
#include <di/function/flip.h>
#include <di/util/declval.h>

namespace di::concepts {
template<typename F, typename T, typename Iter>
concept IndirectlyBinaryRightFoldable =
    IndirectlyBinaryLeftFoldable<decltype(function::flip(util::declval<F>())), T, Iter>;
;
}
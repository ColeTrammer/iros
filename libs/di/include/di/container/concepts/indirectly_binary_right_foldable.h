#pragma once

#include "di/container/concepts/indirectly_binary_left_foldable.h"
#include "di/function/flip.h"
#include "di/util/declval.h"

namespace di::concepts {
namespace detail {
    template<typename F>
    class FlippedHelper {
    private:
        F f;

    public:
        template<typename T, typename U>
        requires(concepts::Invocable<F&, U, T>)
        auto operator()(T&&, U&&) -> meta::InvokeResult<F&, U, T>;
    };
}

template<typename F, typename T, typename Iter>
concept IndirectlyBinaryRightFoldable = IndirectlyBinaryLeftFoldable<detail::FlippedHelper<F>, T, Iter>;
;
}

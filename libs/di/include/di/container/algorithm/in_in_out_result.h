#pragma once

#include "di/meta/operations.h"
#include "di/util/move.h"

namespace di::container {
template<typename In1, typename In2, typename O>
struct InInOutResult {
    template<typename I1, typename I2, typename OO>
    requires(concepts::ConvertibleTo<In1 const&, I1> && concepts::ConvertibleTo<In2 const&, I2> &&
             concepts::ConvertibleTo<O const&, OO>)
    constexpr operator InInOutResult<I1, I2, OO>() const& {
        return { in1, in2, out };
    }

    template<typename I1, typename I2, typename OO>
    requires(concepts::ConvertibleTo<In1, I1> && concepts::ConvertibleTo<In2, I2> && concepts::ConvertibleTo<O, OO>)
    constexpr operator InInOutResult<I1, I2, OO>() && {
        return { util::move(in1), util::move(in2), util::move(out) };
    }

    [[no_unique_address]] In1 in1;
    [[no_unique_address]] In2 in2;
    [[no_unique_address]] O out;
};
}

#pragma once

#include <di/concepts/convertible_to.h>
#include <di/util/move.h>

namespace di::container {
template<typename In1, typename In2>
struct InInResult {
    template<typename I1, typename I2>
    requires(concepts::ConvertibleTo<I1 const&, In1> && concepts::ConvertibleTo<I2 const&, In2>)
    constexpr operator InInResult<I1, I2>() const& {
        return { in1, in2 };
    }

    template<typename I1, typename I2>
    requires(concepts::ConvertibleTo<I1, In1> && concepts::ConvertibleTo<I2, In2>)
    constexpr operator InInResult<I1, I2>() const& {
        return { util::move(in1), util::move(in2) };
    }

    [[no_unique_address]] In1 in1;
    [[no_unique_address]] In2 in2;
};
}

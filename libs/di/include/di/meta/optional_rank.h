#pragma once

#include <di/concepts/optional.h>
#include <di/meta/optional_value.h>
#include <di/meta/size_constant.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct OptionalRankHelper : SizeConstant<0> {};

    template<concepts::Optional T>
    struct OptionalRankHelper<T> : SizeConstant<1 + OptionalRankHelper<OptionalValue<T>>::value> {};
}

template<typename T>
constexpr inline auto OptionalRank = detail::OptionalRankHelper<T>::value;
}

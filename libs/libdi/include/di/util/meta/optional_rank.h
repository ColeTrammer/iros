#pragma once

#include <di/util/concepts/optional.h>
#include <di/util/meta/optional_value.h>
#include <di/util/meta/size_constant.h>

namespace di::util::meta {
namespace detail {
    template<typename T>
    struct OptionalRankHelper : SizeConstant<0> {};

    template<concepts::Optional T>
    struct OptionalRankHelper<T> : SizeConstant<1 + OptionalRankHelper<OptionalValue<T>>::value> {};
}

template<typename T>
constexpr inline auto OptionalRank = detail::OptionalRankHelper<T>::value;
}

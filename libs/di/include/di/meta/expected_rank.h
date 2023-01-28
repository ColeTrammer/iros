#pragma once

#include <di/concepts/expected.h>
#include <di/meta/expected_value.h>
#include <di/meta/size_constant.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct ExpectedRankHelper : SizeConstant<0> {};

    template<concepts::Expected T>
    struct ExpectedRankHelper<T> : SizeConstant<1 + ExpectedRankHelper<ExpectedValue<T>>::value> {};
}

template<typename T>
constexpr inline auto ExpectedRank = detail::ExpectedRankHelper<T>::value;
}

#pragma once

#include <di/concepts/language_array.h>
#include <di/meta/size_constant.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct ArrayRankHelper : SizeConstant<0> {};

    template<typename T>
    struct ArrayRankHelper<T[]> : SizeConstant<1 + ArrayRankHelper<T>::value> {};

    template<typename T, size_t N>
    struct ArrayRankHelper<T[N]> : SizeConstant<1 + ArrayRankHelper<T>::value> {};
}

template<concepts::LanguageArray T>
constexpr inline auto ArrayRank = detail::ArrayRankHelper<T>::value;
}
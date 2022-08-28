#pragma once

#include <di/meta/size_constant.h>
#include <di/types/size_t.h>

namespace di::meta {
namespace detail {
    template<typename T, types::size_t level>
    struct ExtentHelper : SizeConstant<0> {};

    template<typename T>
    struct ExtentHelper<T[], 0> : SizeConstant<0> {};

    template<typename T, types::size_t level>
    struct ExtentHelper<T[], level> : ExtentHelper<T, level - 1> {};

    template<typename T, types::size_t size>
    struct ExtentHelper<T[size], 0> : SizeConstant<size> {};

    template<typename T, types::size_t size, types::size_t level>
    struct ExtentHelper<T[size], level> : ExtentHelper<T, level - 1> {};
}

template<typename T, types::size_t level = 0>
constexpr inline auto Extent = detail::ExtentHelper<T, level>::value;
}

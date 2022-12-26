#pragma once

#include <di/meta/integral_constant.h>
#include <di/types/prelude.h>

namespace di::meta {
template<size_t constant>
using SizeConstant = IntegralConstant<size_t, constant>;

template<size_t constant>
constexpr inline auto size_constant = SizeConstant<constant> {};
}

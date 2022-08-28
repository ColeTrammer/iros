#pragma once

#include <di/meta/integral_constant.h>
#include <di/types/size_t.h>

namespace di::meta {
template<size_t constant>
struct SizeConstant : IntegralConstant<size_t, constant> {};
}

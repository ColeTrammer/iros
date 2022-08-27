#pragma once

#include <di/util/meta/integral_constant.h>
#include <di/util/types/size_t.h>

namespace di::util::meta {
template<size_t constant>
struct SizeConstant : IntegralConstant<size_t, constant> {};
}

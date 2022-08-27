#pragma once

#include <di/util/meta/integral_constant.h>

namespace di::util::meta {
template<bool constant>
struct BoolConstant : IntegralConstant<bool, constant> {};
}

#pragma once

#include <di/meta/integral_constant.h>

namespace di::meta {
template<bool constant>
using BoolConstant = IntegralConstant<bool, constant>;
}

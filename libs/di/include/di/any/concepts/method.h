#pragma once

#include "di/any/types/prelude.h"
#include "di/meta/core.h"

namespace di::concepts {
template<typename T>
concept Method = InstanceOf<T, types::Method>;
}

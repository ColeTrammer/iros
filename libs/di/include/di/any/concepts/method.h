#pragma once

#include <di/any/types/prelude.h>
#include <di/concepts/instance_of.h>

namespace di::concepts {
template<typename T>
concept Method = InstanceOf<T, types::Method>;
}
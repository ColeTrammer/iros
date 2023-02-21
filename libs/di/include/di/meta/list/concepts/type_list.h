#pragma once

#include <di/concepts/instance_of.h>
#include <di/meta/list/list_forward_declation.h>

namespace di::concepts {
template<typename T>
concept TypeList = InstanceOf<T, meta::List>;
}

#pragma once

#include "di/container/concepts/container.h"
#include "di/container/meta/container_value.h"
#include "di/meta/core.h"

namespace di::concepts {
template<typename T, typename Value>
concept ContainerOf = concepts::Container<T> && concepts::SameAs<meta::ContainerValue<T>, Value>;
}

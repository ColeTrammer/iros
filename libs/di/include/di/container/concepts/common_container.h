#pragma once

#include <di/container/concepts/container.h>
#include <di/container/meta/container_iterator.h>
#include <di/container/meta/container_sentinel.h>
#include <di/meta/core.h>

namespace di::concepts {
template<typename T>
concept CommonContainer = Container<T> && SameAs<meta::ContainerIterator<T>, meta::ContainerSentinel<T>>;
}

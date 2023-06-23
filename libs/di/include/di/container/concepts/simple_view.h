#pragma once

#include <di/container/concepts/container.h>
#include <di/container/concepts/view.h>
#include <di/container/meta/container_iterator.h>
#include <di/container/meta/container_sentinel.h>
#include <di/meta/core.h>

namespace di::concepts {
template<typename T>
concept SimpleView =
    View<T> && Container<T const> && SameAs<meta::ContainerIterator<T>, meta::ContainerIterator<T const>> &&
    SameAs<meta::ContainerSentinel<T>, meta::ContainerSentinel<T const>>;
}

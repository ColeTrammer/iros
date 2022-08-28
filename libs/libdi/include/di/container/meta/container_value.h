#pragma once

#include <di/container/meta/container_iterator.h>
#include <di/container/meta/iterator_value.h>

namespace di::meta {
template<typename T>
using ContainerValue = IteratorValue<ContainerIterator<T>>;
}

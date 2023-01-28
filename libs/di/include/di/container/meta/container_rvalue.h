#pragma once

#include <di/container/meta/container_iterator.h>
#include <di/container/meta/iterator_rvalue.h>

namespace di::meta {
template<typename T>
using ContainerRValue = IteratorRValue<ContainerIterator<T>>;
}

#pragma once

#include <di/container/meta/container_iterator.h>
#include <di/container/meta/iterator_reference.h>

namespace di::meta {
template<typename T>
using ContainerReference = IteratorReference<ContainerIterator<T>>;
}

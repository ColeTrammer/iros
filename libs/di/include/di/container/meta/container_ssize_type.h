#pragma once

#include <di/container/meta/container_iterator.h>
#include <di/container/meta/iterator_ssize_type.h>

namespace di::meta {
template<typename T>
using ContainerSSizeType = IteratorSSizeType<ContainerIterator<T>>;
}

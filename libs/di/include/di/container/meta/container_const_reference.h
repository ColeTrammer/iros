#pragma once

#include "di/container/concepts/container.h"
#include "di/container/meta/container_iterator.h"
#include "di/container/meta/iterator_const_reference.h"

namespace di::meta {
template<concepts::Container Con>
using ContainerConstReference = IteratorConstReference<ContainerIterator<Con>>;
}

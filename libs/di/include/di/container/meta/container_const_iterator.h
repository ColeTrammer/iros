#pragma once

#include <di/container/concepts/container.h>
#include <di/container/meta/const_iterator.h>
#include <di/container/meta/container_iterator.h>

namespace di::meta {
template<concepts::Container Con>
using ContainerConstIterator = ConstIterator<ContainerIterator<Con>>;
}

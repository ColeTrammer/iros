#pragma once

#include <di/container/concepts/container.h>
#include <di/container/concepts/output_iterator.h>
#include <di/container/meta/container_iterator.h>

namespace di::concepts {
template<typename Con, typename T>
concept OutputContainer = Container<Con> && OutputIterator<meta::ContainerIterator<Con>, T>;
}

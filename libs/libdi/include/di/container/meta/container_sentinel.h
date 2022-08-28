#pragma once

#include <di/container/interface/end.h>
#include <di/util/declval.h>

namespace di::meta {
template<typename T>
using ContainerSentinel = decltype(container::end(util::declval<T&>()));
}

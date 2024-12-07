#pragma once

#include "di/container/interface/begin.h"
#include "di/util/declval.h"

namespace di::meta {
template<typename T>
using ContainerIterator = decltype(container::begin(util::declval<T&>()));
}

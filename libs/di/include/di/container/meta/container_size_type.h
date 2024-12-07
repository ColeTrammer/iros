#pragma once

#include "di/container/interface/size.h"
#include "di/util/declval.h"

namespace di::meta {
template<typename T>
using ContainerSizeType = decltype(container::size(util::declval<T>()));
}

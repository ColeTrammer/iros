#pragma once

#include <di/container/interface/ssize.h>
#include <di/util/declval.h>

namespace di::meta {
template<typename T>
using ContainerSSizeType = decltype(container::ssize(util::declval<T>()));
}

#pragma once

#include <di/util/meta/remove_const.h>
#include <di/util/meta/remove_volatile.h>

namespace di::util::meta {
template<typename T>
using RemoveCV = RemoveConst<RemoveVolatile<T>>;
}

#pragma once

#include <di/meta/remove_const.h>
#include <di/meta/remove_volatile.h>

namespace di::meta {
template<typename T>
using RemoveCV = RemoveConst<RemoveVolatile<T>>;
}

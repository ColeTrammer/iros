#pragma once

#include "di/util/declval.h"

namespace di::meta {
template<typename T>
using IteratorReference = decltype(*util::declval<T const&>());
}

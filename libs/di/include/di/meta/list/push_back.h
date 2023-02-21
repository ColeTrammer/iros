#pragma once

#include <di/meta/list/concat.h>

namespace di::meta {
template<concepts::TypeList L, typename T>
using PushBack = Concat<L, List<T>>;
}

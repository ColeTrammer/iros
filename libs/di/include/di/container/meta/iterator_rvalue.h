#pragma once

#include <di/container/iterator/iterator_move.h>
#include <di/util/declval.h>

namespace di::meta {
template<typename T>
requires(requires { container::iterator_move(util::declval<T&>()); })
using IteratorRValue = decltype(container::iterator_move(util::declval<T&>()));
}

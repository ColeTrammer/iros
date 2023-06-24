#pragma once

#include <di/container/iterator/iterator_ssize_type.h>
#include <di/meta/core.h>

namespace di::meta {
template<typename T>
using IteratorSSizeType = decltype(container::iterator_ssize_type(types::in_place_type<meta::RemoveCVRef<T>>));
}

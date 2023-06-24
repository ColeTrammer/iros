#pragma once

#include <di/container/iterator/iterator_category.h>
#include <di/meta/core.h>

namespace di::meta {
template<typename T>
using IteratorCategory = decltype(container::iterator_category(types::in_place_type<meta::RemoveCVRef<T>>));
}

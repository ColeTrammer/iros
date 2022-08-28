#pragma once

#include <di/container/iterator/iterator_value.h>
#include <di/meta/remove_cvref.h>

namespace di::meta {
template<typename T>
using IteratorValue = decltype(container::iterator_value(types::in_place_type<meta::RemoveCVRef<T>>));
}

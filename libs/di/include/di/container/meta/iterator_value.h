#pragma once

#include <di/container/iterator/iterator_value.h>
#include <di/meta/core.h>
#include <di/meta/remove_cvref.h>

namespace di::meta {
template<typename T>
using IteratorValue = meta::Type<decltype(container::iterator_value(types::in_place_type<meta::RemoveCVRef<T>>))>;
}

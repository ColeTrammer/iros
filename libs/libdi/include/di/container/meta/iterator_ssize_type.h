#pragma once

#include <di/container/iterator/iterator_ssize_type.h>
#include <di/meta/remove_cvref.h>

namespace di::meta {
template<typename T>
using IteratorSSizeType = decltype(container::iterator_ssize_type(types::in_place_type<meta::RemoveCVRef<T>>));
}

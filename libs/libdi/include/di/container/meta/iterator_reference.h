#pragma once

#include <di/container/iterator/iterator_reference.h>
#include <di/meta/remove_cvref.h>

namespace di::meta {
template<typename T>
using IteratorReference = decltype(container::iterator_reference(types::in_place_type<meta::RemoveCVRef<T>>));
}

#pragma once

#include <di/meta/remove_all_extents.h>
#include <di/meta/remove_cv.h>

namespace di::concepts {
template<typename T>
concept UniqueObjectRepresentation = __has_unique_object_representations(meta::RemoveCV<meta::RemoveAllExtents<T>>);
}
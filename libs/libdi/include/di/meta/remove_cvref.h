#pragma once

#include <di/meta/remove_cv.h>
#include <di/meta/remove_reference.h>

namespace di::meta {
template<typename T>
using RemoveCVRef = RemoveCV<RemoveReference<T>>;
}

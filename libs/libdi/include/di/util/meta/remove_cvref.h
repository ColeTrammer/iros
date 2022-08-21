#pragma once

#include <di/util/meta/remove_cv.h>
#include <di/util/meta/remove_reference.h>

namespace di::util::meta {
template<typename T>
using RemoveCVRef = RemoveCV<RemoveReference<T>>;
}

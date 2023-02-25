#pragma once

#include <di/meta/remove_cv.h>

namespace di::concepts {
template<typename T>
concept Aggregate = __is_aggregate(meta::RemoveCV<T>);
}

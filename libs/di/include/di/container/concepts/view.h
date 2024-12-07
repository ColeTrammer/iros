#pragma once

#include "di/container/concepts/container.h"
#include "di/container/interface/enable_view.h"
#include "di/meta/core.h"
#include "di/meta/operations.h"

namespace di::concepts {
template<typename T>
concept View = Container<T> && Movable<T> && container::enable_view(types::in_place_type<meta::RemoveCVRef<T>>);
}

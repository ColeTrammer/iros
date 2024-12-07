#pragma once

#include "di/container/concepts/container.h"
#include "di/container/interface/enable_borrowed_container.h"

namespace di::concepts {
template<typename T>
concept BorrowedContainer = Container<T> && container::enable_borrowed_container(types::in_place_type<T>);
}

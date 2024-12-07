#pragma once

#include "di/container/interface/empty.h"

namespace di::concepts {
template<typename T>
concept HasEmptyContainer = requires(T& container) { container::empty(container); };
}

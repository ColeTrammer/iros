#pragma once

#include <di/container/view/empty_view.h>

namespace di::container::view {
template<typename T>
constexpr inline auto empty = EmptyView<T> {};
}

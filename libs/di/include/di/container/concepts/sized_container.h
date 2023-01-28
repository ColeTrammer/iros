#pragma once

#include <di/container/concepts/container.h>
#include <di/container/interface/size.h>

namespace di::concepts {
template<typename T>
concept SizedContainer = Container<T> && requires(T& value) { container::size(value); };
}

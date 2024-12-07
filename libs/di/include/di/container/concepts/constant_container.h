#pragma once

#include "di/container/concepts/constant_iterator.h"
#include "di/container/concepts/input_container.h"
#include "di/container/meta/container_iterator.h"

namespace di::concepts {
template<typename Con>
concept ConstantContainer = InputContainer<Con> && ConstantIterator<meta::ContainerIterator<Con>>;
}

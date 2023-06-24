#pragma once

#include <di/container/concepts/input_container.h>
#include <di/container/meta/container_reference.h>
#include <di/meta/operations.h>

namespace di::concepts {
template<typename Con, typename Value>
concept ContainerCompatible = InputContainer<Con> && ConvertibleTo<meta::ContainerReference<Con>, Value>;
}

#pragma once

#include <di/concepts/convertible_to.h>
#include <di/container/concepts/input_container.h>
#include <di/container/meta/container_reference.h>

namespace di::concepts {
template<typename Con, typename Value>
concept ContainerCompatible = InputContainer<Con> && ConvertibleTo<meta::ContainerReference<Con>, Value>;
}

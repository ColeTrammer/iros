#pragma once

#include <di/container/concepts/constant_container.h>
#include <di/container/concepts/input_container.h>
#include <di/util/as_const.h>

namespace di::container::detail {
template<concepts::InputContainer Con>
constexpr auto& possibly_const_container(Con& container) {
    if constexpr (concepts::ConstantContainer<Con const> && !concepts::ConstantContainer<Con>) {
        return util::as_const(container);
    } else {
        return container;
    }
}
}
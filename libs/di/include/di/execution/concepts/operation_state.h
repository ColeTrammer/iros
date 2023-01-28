#pragma once

#include <di/concepts/destructible.h>
#include <di/concepts/object.h>
#include <di/execution/interface/start.h>

namespace di::concepts {
template<typename T>
concept OperationState =
    concepts::Destructible<T> && concepts::Object<T> && requires(T& op_state) { execution::start(op_state); };
}
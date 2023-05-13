#pragma once

#include <di/concepts/object.h>
#include <di/execution/concepts/queryable.h>
#include <di/execution/interface/start.h>

namespace di::concepts {
template<typename T>
concept OperationState =
    concepts::Queryable<T> && concepts::Object<T> && requires(T& op_state) { execution::start(op_state); };
}

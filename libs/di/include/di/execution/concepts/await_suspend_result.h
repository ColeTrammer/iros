#pragma once

#include <di/concepts/instance_of.h>
#include <di/concepts/one_of.h>
#include <di/util/coroutine.h>

namespace di::concepts {
// The result of await_suspend() can either be void, a bool, or a coroutine handle.
template<typename T>
concept AwaitSuspendResult = concepts::OneOf<T, void, bool> || concepts::InstanceOf<T, std::coroutine_handle>;
}

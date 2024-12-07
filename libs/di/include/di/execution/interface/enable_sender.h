#pragma once

#include "di/execution/concepts/is_awaitable.h"
#include "di/execution/coroutine/env_promise.h"
#include "di/execution/types/empty_env.h"

namespace di::execution {
template<typename S>
constexpr inline bool enable_sender = requires { typename S::is_sender; };

template<concepts::IsAwaitable<EnvPromise<types::EmptyEnv>> S>
constexpr inline bool enable_sender<S> = true;
}

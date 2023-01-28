#pragma once

#include <di/execution/concepts/async_destroyable.h>
#include <di/execution/interface/async_create.h>

namespace di::concepts {
template<typename T, typename... Args>
concept AsyncCreatableFrom =
    requires(Args&&... args) { execution::async_create<T>(util::forward<Args>(args)...); } && AsyncDestroyable<T>;
}
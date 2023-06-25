#pragma once

#include <di/execution/interface/get_env.h>
#include <di/execution/receiver/is_receiver.h>
#include <di/meta/core.h>
#include <di/meta/operations.h>

namespace di::concepts {
template<typename T>
concept Receiver = execution::enable_receiver<meta::RemoveCVRef<T>> && requires(meta::RemoveCVRef<T> const& receiver) {
    { execution::get_env(receiver) } -> Queryable;
} && MoveConstructible<meta::RemoveCVRef<T>> && concepts::ConstructibleFrom<meta::RemoveCVRef<T>, T>;
}

namespace di {
using concepts::Receiver;
}

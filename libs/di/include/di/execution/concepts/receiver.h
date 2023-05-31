#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/move_constructible.h>
#include <di/execution/interface/get_env.h>
#include <di/execution/receiver/is_receiver.h>
#include <di/meta/remove_cvref.h>

namespace di::concepts {
template<typename T>
concept Receiver = execution::enable_receiver<meta::RemoveCVRef<T>> && requires(meta::RemoveCVRef<T> const& receiver) {
    { execution::get_env(receiver) } -> Queryable;
} && MoveConstructible<meta::RemoveCVRef<T>> && concepts::ConstructibleFrom<meta::RemoveCVRef<T>, T>;
}

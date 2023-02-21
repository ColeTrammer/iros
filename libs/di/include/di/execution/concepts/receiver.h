#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/move_constructible.h>
#include <di/execution/query/get_env.h>
#include <di/meta/remove_cvref.h>

namespace di::concepts {
template<typename T>
concept Receiver = MoveConstructible<meta::RemoveCVRef<T>> && concepts::ConstructibleFrom<meta::RemoveCVRef<T>, T> &&
                   requires(meta::RemoveCVRef<T> const& receiver) { execution::get_env(receiver); };
}

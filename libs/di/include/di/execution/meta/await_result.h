#pragma once

#include "di/execution/concepts/is_awaitable.h"
#include "di/meta/util.h"

namespace di::meta {
template<typename Awaitable, typename Promise = void>
requires(concepts::IsAwaitable<Awaitable, Promise>)
using AwaitResult =
    decltype(concepts::detail::get_awaiter(util::declval<Awaitable>(), util::declval<Promise*>()).await_resume());
}

#pragma once

#include <di/execution/concepts/awaitable.h>
#include <di/meta/add_lvalue_reference.h>

namespace di::meta {
template<typename Awaitable, typename Promise = void>
requires(concepts::Awaitable<Awaitable, Promise>)
using AwaitResult =
    decltype(concepts::detail::get_awaiter(util::declval<Awaitable>(), util::declval<Promise*>()).await_resume());
}

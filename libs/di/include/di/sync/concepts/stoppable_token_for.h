#pragma once

#include "di/function/invoke.h"
#include "di/meta/operations.h"
#include "di/sync/concepts/stoppable_token.h"

namespace di::concepts {
template<typename T, typename Callback, typename Init = Callback>
concept StoppableTokenFor =
    StoppableToken<T> && Invocable<Callback> && requires { typename T::template CallbackType<Callback>; } &&
    ConstructibleFrom<Callback, Init> && ConstructibleFrom<typename T::template CallbackType<Callback>, T, Init> &&
    ConstructibleFrom<typename T::template CallbackType<Callback>, T&, Init> &&
    ConstructibleFrom<typename T::template CallbackType<Callback>, T const, Init> &&
    ConstructibleFrom<typename T::template CallbackType<Callback>, T const&, Init>;
}

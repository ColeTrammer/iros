#pragma once

#include <di/execution/concepts/sender_to.h>
#include <di/execution/concepts/single_sender.h>
#include <di/execution/meta/env_of.h>

namespace di::execution::as_awaitable_ns {
template<typename Send, typename Promise>
struct AwaitableReceiver;
}

namespace di::concepts {
template<typename Send, typename Promise>
concept AwaitableSender =
    SingleSender<Send, meta::EnvOf<Promise>> && SenderTo<Send, execution::as_awaitable_ns::AwaitableReceiver<Send, Promise>> &&
    requires(Promise& promise, Error error) {
        { promise.unhandled_stopped() } -> concepts::ConvertibleTo<CoroutineHandle<>>;
        { promise.unhandled_error(util::move(error)) } -> concepts::ConvertibleTo<CoroutineHandle<>>;
    };
}
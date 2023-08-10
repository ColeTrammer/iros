#pragma once

#include <di/execution/concepts/operation_state.h>
#include <di/execution/concepts/receiver_of.h>
#include <di/execution/concepts/sender.h>
#include <di/execution/interface/connect_awaitable.h>
#include <di/execution/meta/completion_signatures_of.h>
#include <di/execution/meta/env_of.h>
#include <di/execution/query/is_debug_env.h>
#include <di/function/tag_invoke.h>

namespace di::execution {
namespace detail {
    struct ConnectFunction;

    template<typename Sender, typename Receiver>
    concept CustomConnect =
        /* concepts::Sender<Sender> &&
        concepts::ReceiverOf<Receiver, meta::CompletionSignaturesOf<Sender, meta::EnvOf<Receiver>>> && */
        concepts::TagInvocable<ConnectFunction, Sender, Receiver>;

    template<typename Sender, typename Receiver>
    concept AwaitableConnect = requires(Sender&& sender, Receiver&& receiver) {
        connect_awaitable_ns::connect_awaitable(util::forward<Sender>(sender), util::forward<Receiver>(receiver));
    };

    struct ConnectFunction {
        template<typename Sender, typename Receiver>
        requires(CustomConnect<Sender, Receiver> || AwaitableConnect<Sender, Receiver> ||
                 concepts::DebugEnv<meta::EnvOf<Receiver>>)
        constexpr concepts::OperationState auto operator()(Sender&& sender, Receiver&& receiver) const {
            if constexpr (CustomConnect<Sender, Receiver>) {
                return function::tag_invoke(*this, util::forward<Sender>(sender), util::forward<Receiver>(receiver));
            } else if constexpr (AwaitableConnect<Sender, Receiver>) {
                return connect_awaitable_ns::connect_awaitable(util::forward<Sender>(sender),
                                                               util::forward<Receiver>(receiver));
            } else {
                return function::tag_invoke(*this, util::forward<Sender>(sender), util::forward<Receiver>(receiver));
            }
        }
    };
}

constexpr inline auto connect = detail::ConnectFunction {};
}

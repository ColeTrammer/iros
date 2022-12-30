#pragma once

#include <di/execution/concepts/operation_state.h>
#include <di/execution/concepts/receiver_of.h>
#include <di/execution/concepts/sender.h>
#include <di/execution/meta/completion_signatures_of.h>
#include <di/execution/meta/env_of.h>
#include <di/function/tag_invoke.h>

namespace di::execution {
namespace detail {
    struct ConnectFunction;

    template<typename Sender, typename Receiver>
    concept CustomConnect = concepts::Sender<Sender, meta::EnvOf<Receiver>> &&
                            concepts::ReceiverOf<Receiver, meta::CompletionSignaturesOf<Sender, meta::EnvOf<Receiver>>> &&
                            concepts::TagInvocable<ConnectFunction, Sender, Receiver>;

    struct ConnectFunction {
        template<typename Sender, typename Receiver>
        requires(CustomConnect<Sender, Receiver>)
        constexpr concepts::OperationState auto operator()(Sender&& sender, Receiver&& receiver) const {
            // TODO: handle connecting to a awaitable.
            return function::tag_invoke(*this, util::forward<Sender>(sender), util::forward<Receiver>(receiver));
        }
    };
}

constexpr inline auto connect = detail::ConnectFunction {};
}
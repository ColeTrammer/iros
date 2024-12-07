#pragma once

#include "di/execution/concepts/receiver_of.h"
#include "di/execution/concepts/sender_in.h"
#include "di/execution/interface/connect.h"
#include "di/execution/meta/completion_signatures_of.h"
#include "di/execution/meta/env_of.h"

namespace di::concepts {
template<typename Send, typename Recv>
concept SenderTo =
    SenderIn<Send, meta::EnvOf<Recv>> && ReceiverOf<Recv, meta::CompletionSignaturesOf<Send, meta::EnvOf<Recv>>> &&
    requires(Send&& sender, Recv&& receiver) {
        execution::connect(util::forward<Send>(sender), util::forward<Recv>(receiver));
    };
}

namespace di {
using concepts::SenderTo;
}

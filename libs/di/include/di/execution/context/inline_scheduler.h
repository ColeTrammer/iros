#pragma once

#include "di/execution/concepts/operation_state.h"
#include "di/execution/concepts/receiver.h"
#include "di/execution/concepts/scheduler.h"
#include "di/execution/interface/connect.h"
#include "di/execution/interface/get_env.h"
#include "di/execution/query/get_completion_scheduler.h"
#include "di/execution/types/prelude.h"
#include "di/function/tag_invoke.h"

namespace di::execution {
struct InlineScheduler {
private:
    template<typename Rec>
    struct OperationStateT {
        struct Type {
            [[no_unique_address]] Rec receiver;

            friend void tag_invoke(types::Tag<start>, Type& self) { execution::set_value(util::move(self.receiver)); }
        };
    };

    template<concepts::Receiver Rec>
    using OperationState = meta::Type<OperationStateT<Rec>>;

    struct Sender {
        using is_sender = void;

        using CompletionSignatures = types::CompletionSignatures<SetValue()>;

    private:
        template<typename Rec>
        auto do_connect(Rec receiver) {
            return OperationState<Rec> { util::move(receiver) };
        }

        template<concepts::ReceiverOf<CompletionSignatures> Rec>
        friend auto tag_invoke(types::Tag<connect>, Sender self, Rec receiver) {
            return self.do_connect(util::move(receiver));
        }

        struct Env {
            template<typename CPO>
            friend auto tag_invoke(GetCompletionScheduler<CPO>, Env) {
                return InlineScheduler {};
            }
        };

        friend auto tag_invoke(types::Tag<get_env>, Sender) { return Env {}; }
    };

public:
    InlineScheduler() = default;

private:
    friend auto operator==(InlineScheduler const&, InlineScheduler const&) -> bool = default;
    friend auto tag_invoke(types::Tag<schedule>, InlineScheduler const&) { return Sender {}; }
};
}

namespace di {
using execution::InlineScheduler;
}

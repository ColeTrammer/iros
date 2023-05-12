#pragma once

#include <di/execution/concepts/prelude.h>
#include <di/execution/interface/prelude.h>
#include <di/execution/meta/prelude.h>
#include <di/execution/receiver/prelude.h>
#include <di/execution/types/prelude.h>

namespace di::execution {
namespace just_void_or_stopped_ns {
    template<typename Rec>
    class OperationStateT {
        struct Type {
        public:
            bool should_stop;
            [[no_unique_address]] Rec receiver;

        private:
            constexpr friend void tag_invoke(types::Tag<execution::start>, Type& self) {
                if (self.should_stop) {
                    execution::set_stopped(util::move(self.receiver));
                } else {
                    execution::set_value(util::move(self.receiver));
                }
            }
        };
    };

    template<concepts::Receiver Rec>
    using OperationState = meta::Type<OperationStateT<Rec>>;

    struct SenderT {
        struct Type {
        public:
            using is_sender = void;

            using CompletionSignatures = types::CompletionSignatures<SetValue(), SetStopped()>;

            bool should_stop;

        private:
            template<concepts::ReceiverOf<CompletionSignatures> Rec>
            constexpr friend auto tag_invoke(types::Tag<execution::connect>, Type self, Rec receiver) {
                return OperationState<Rec> { self.should_stop, util::move(receiver) };
            }
        };
    };

    using Sender = meta::Type<SenderT>;

    struct Function {
        concepts::Sender auto operator()(bool should_stop) const { return Sender { should_stop }; }
    };
}

constexpr inline auto just_void_or_stopped = just_void_or_stopped_ns::Function {};
}

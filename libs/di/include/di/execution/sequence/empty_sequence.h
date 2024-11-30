#pragma once

#include <di/execution/concepts/receiver.h>
#include <di/execution/concepts/receiver_of.h>
#include <di/execution/interface/get_env.h>
#include <di/execution/interface/start.h>
#include <di/execution/query/is_always_lockstep_sequence.h>
#include <di/execution/query/make_env.h>
#include <di/execution/receiver/set_value.h>
#include <di/execution/sequence/sequence_sender.h>
#include <di/execution/types/empty_env.h>
#include <di/function/tag_invoke.h>
#include <di/meta/core.h>
#include <di/util/immovable.h>
#include <di/util/move.h>

namespace di::execution {
namespace empty_sequence_ns {
    template<typename Rec>
    struct OperationStateT {
        struct Type : di::Immovable {
        public:
            constexpr explicit Type(Rec receiver) : m_receiver(di::move(receiver)) {}

            friend void tag_invoke(types::Tag<start>, Type& self) { set_value(util::move(self.m_receiver)); }

        private:
            [[no_unique_address]] Rec m_receiver;
        };
    };

    template<concepts::Receiver Rec>
    using OperationState = meta::Type<OperationStateT<Rec>>;

    struct Sender {
        using is_sender = SequenceTag;

        using CompletionSignatures = types::CompletionSignatures<>;

        template<concepts::SubscriberOf<CompletionSignatures> Rec>
        friend auto tag_invoke(types::Tag<subscribe>, Sender, Rec receiver) {
            return OperationState<Rec> { util::move(receiver) };
        };

        friend auto tag_invoke(Tag<get_env>, Sender) {
            return make_env(empty_env, with(get_sequence_cardinality, c_<0ZU>));
        }
    };

    struct Function {
        auto operator()() const { return Sender {}; }
    };
}

/// @brief A sequence sender that completes immediately without ever sending a value.
constexpr inline auto empty_sequence = empty_sequence_ns::Function {};
}

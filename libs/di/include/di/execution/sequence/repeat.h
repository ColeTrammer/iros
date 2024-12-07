#pragma once

#include "di/execution/concepts/sender.h"
#include "di/execution/interface/connect.h"
#include "di/execution/interface/get_env.h"
#include "di/execution/meta/connect_result.h"
#include "di/execution/meta/env_of.h"
#include "di/execution/meta/make_completion_signatures.h"
#include "di/execution/meta/stop_token_of.h"
#include "di/execution/query/get_completion_signatures.h"
#include "di/execution/query/is_always_lockstep_sequence.h"
#include "di/execution/query/make_env.h"
#include "di/execution/receiver/set_stopped.h"
#include "di/execution/receiver/set_value.h"
#include "di/execution/sequence/sequence_sender.h"
#include "di/execution/types/completion_signuatures.h"
#include "di/function/tag_invoke.h"
#include "di/meta/constexpr.h"
#include "di/meta/core.h"
#include "di/meta/operations.h"
#include "di/meta/util.h"
#include "di/platform/compiler.h"
#include "di/sync/concepts/stoppable_token.h"
#include "di/sync/concepts/unstoppable_token.h"
#include "di/util/as_const.h"
#include "di/util/defer_construct.h"
#include "di/util/immovable.h"
#include "di/util/rebindable_box.h"
#include "di/vocab/optional/prelude.h"

namespace di::execution {
namespace repeat_ns {
    template<typename Op, typename Rec>
    struct ReceiverT {
        struct Type {
            using is_receiver = void;

            Op* op;

            friend void tag_invoke(Tag<set_value>, Type&& self) { self.op->did_set_value(); }

            friend void tag_invoke(Tag<set_stopped>, Type&& self) { self.op->did_set_stopped(); }

            friend auto tag_invoke(Tag<get_env>, Type const& self) -> MakeEnv<meta::EnvOf<Rec>> {
                return make_env(get_env(self.op->receiver()));
            }
        };
    };

    template<typename Op, typename Rec>
    using Receiver = meta::Type<ReceiverT<Op, Rec>>;

    template<typename Send, typename Rec>
    struct OperationStateT {
        struct Type : util::Immovable {
        public:
            using NextSend = meta::NextSenderOf<Rec, Send>;
            using Re = Receiver<Type, Rec>;
            using Op = meta::ConnectResult<NextSend, Re>;
            using StopToken = meta::StopTokenOf<meta::EnvOf<Rec>>;

            explicit Type(Send sender, Rec reciever)
                : m_sender(di::move(sender))
                , m_receiver(di::move(reciever))
                , m_stop_token(get_stop_token(get_env(m_receiver))) {}

            void did_set_value() {
                if constexpr (!concepts::UnstoppableToken<StopToken>) {
                    if (m_stop_token.stop_requested()) {
                        set_stopped(di::move(m_receiver));
                        return;
                    }
                }

                auto& op = m_operation.emplace(DeferConstruct([&] {
                    return connect(set_next(m_receiver, auto(di::as_const(m_sender))), Receiver<Type, Rec>(this));
                }));
                start(op);
            }

            void did_set_stopped() { set_value(di::move(m_receiver)); }

            auto receiver() const& { return m_receiver; }

        private:
            friend void tag_invoke(Tag<start>, Type& self) { self.did_set_value(); }

            [[no_unique_address]] Send m_sender;
            [[no_unique_address]] Rec m_receiver;
            [[no_unique_address]] StopToken m_stop_token;
            DI_IMMOVABLE_NO_UNIQUE_ADDRESS Optional<Op> m_operation;
        };
    };

    template<typename Send, typename Rec>
    using OperationState = meta::Type<OperationStateT<Send, Rec>>;

    template<typename Send, typename Env>
    using Sigs =
        meta::MakeCompletionSignatures<Send, MakeEnv<Env>,
                                       meta::Conditional<concepts::UnstoppableToken<meta::StopTokenOf<Env>>,
                                                         CompletionSignatures<SetStopped()>, CompletionSignatures<>>>;

    template<typename Send>
    struct SequenceT {
        struct Type {
            using is_sender = SequenceTag;

            Send sender;

            template<concepts::Like<Type> Self, typename Env>
            friend auto tag_invoke(Tag<get_completion_signatures>, Self&&, Env&&) -> Sigs<Send, Env> {
                return {};
            }

            template<concepts::Like<Type> Self, typename Rec>
            requires(concepts::SubscriberOf<Rec, Sigs<Send, meta::EnvOf<Rec>>>)
            friend auto tag_invoke(Tag<subscribe>, Self&& self, Rec receiver) {
                return OperationState<Send, Rec>(di::forward<Self>(self).sender, di::move(receiver));
            }

            friend auto tag_invoke(Tag<get_env>, Type const& self) {
                return make_env(get_env(self.sender), with(is_always_lockstep_sequence, c_<true>));
            }
        };
    };

    template<typename Send>
    using Sequence = meta::Type<SequenceT<meta::RemoveCVRef<Send>>>;

    struct Function {
        template<concepts::Sender Send>
        requires(concepts::CopyConstructible<Send>)
        auto operator()(Send&& sender) const {
            if constexpr (concepts::TagInvocable<Function, Send>) {
                static_assert(concepts::SequenceSender<meta::TagInvokeResult<Function, Send>>,
                              "Customizations of repeat() must return a sequence sender.");
            } else {
                return Sequence<Send>(di::forward<Send>(sender));
            }
        }
    };
}

/// @brief Transform a copyable sender into an infinite sequence sender.
///
/// @param sender The sender to repeat.
///
/// @return A sequence sender that will repeat the input sender infinitely.
constexpr inline auto repeat = repeat_ns::Function {};
}

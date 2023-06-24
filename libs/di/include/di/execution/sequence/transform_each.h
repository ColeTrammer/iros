#pragma once

#include <di/execution/concepts/sender.h>
#include <di/execution/interface/get_env.h>
#include <di/execution/interface/start.h>
#include <di/execution/meta/completion_signatures_of.h>
#include <di/execution/meta/env_of.h>
#include <di/execution/query/get_completion_signatures.h>
#include <di/execution/query/make_env.h>
#include <di/execution/receiver/receiver_adaptor.h>
#include <di/execution/sequence/sequence_sender.h>
#include <di/function/curry_back.h>
#include <di/function/invoke.h>
#include <di/function/tag_invoke.h>
#include <di/meta/core.h>
#include <di/meta/util.h>
#include <di/platform/compiler.h>
#include <di/util/addressof.h>
#include <di/util/immovable.h>

namespace di::execution {
namespace transform_each_ns {
    template<typename Fun, typename Rec>
    struct DataT {
        struct Type {
            [[no_unique_address]] Fun transformer;
            [[no_unique_address]] Rec receiver;
        };
    };

    template<concepts::MovableValue Fun, concepts::Receiver Rec>
    using Data = meta::Type<DataT<meta::Decay<Fun>, Rec>>;

    template<typename Fun, typename Rec>
    struct ReceiverT {
        struct Type : ReceiverAdaptor<Type> {
        private:
            using Base = ReceiverAdaptor<Type>;
            friend Base;

        public:
            explicit Type(Data<Fun, Rec>* data) : m_data(data) {}

            Rec& base() & { return m_data->receiver; }
            Rec const& base() const& { return m_data->receiver; }
            Rec&& base() && { return util::move(m_data->receiver); }

        private:
            template<concepts::Sender Next>
            requires(concepts::Invocable<Fun&, Next>)
            concepts::NextSender auto set_next(Next&& next) & {
                return execution::set_next(this->base(),
                                           function::invoke(m_data->transformer, util::forward<Next>(next)));
            }

            Data<Fun, Rec>* m_data;
        };
    };

    template<concepts::MovableValue Fun, concepts::Receiver Rec>
    using Receiver = meta::Type<ReceiverT<meta::Decay<Fun>, Rec>>;

    template<typename Seq, typename Fun, typename Rec>
    struct OperationStateT {
        struct Type : util::Immovable {
        public:
            using Receiver = transform_each_ns::Receiver<Fun, Rec>;
            using Op = meta::SubscribeResult<Seq, Receiver>;
            using Data = transform_each_ns::Data<Fun, Rec>;

            explicit Type(Seq&& sequence, Fun&& transformer, Rec receiver)
                : m_data(util::forward<Fun>(transformer), util::move(receiver))
                , m_op(subscribe(util::forward<Seq>(sequence), Receiver(util::addressof(m_data)))) {}

        private:
            friend void tag_invoke(types::Tag<start>, Type& self) { start(self.m_op); }

            [[no_unique_address]] Data m_data;
            DI_IMMOVABLE_NO_UNIQUE_ADDRESS Op m_op;
        };
    };

    template<concepts::Sender Seq, concepts::MovableValue Fun, concepts::Receiver Rec>
    using OperationState = meta::Type<OperationStateT<Seq, Fun, Rec>>;

    template<typename Seq, typename Fun, typename Env>
    using Signatures = meta::CompletionSignaturesOf<
        meta::InvokeResult<Fun&, dummy_ns::DummySenderOf<meta::CompletionSignaturesOf<Seq, Fun>>>, Env>;

    template<typename Seq, typename Fun>
    struct SequenceT {
        struct Type {
            using is_sender = SequenceTag;

            [[no_unique_address]] Seq sequence;
            [[no_unique_address]] Fun transformer;

            template<concepts::RemoveCVRefSameAs<Type> Self, typename Rec>
            requires(concepts::DecayConstructible<meta::Like<Self, Fun>> &&
                     concepts::SubscriberOf<Rec, Signatures<meta::Like<Self, Seq>, Fun, MakeEnv<meta::EnvOf<Rec>>>>)
            friend auto tag_invoke(types::Tag<subscribe>, Self&& self, Rec receiver) {
                return OperationState<meta::Like<Self, Seq>, meta::Like<Self, Fun>, Rec>(
                    util::forward<Self>(self).sequence, util::forward<Self>(self).transformer, util::move(receiver));
            }

            template<concepts::RemoveCVRefSameAs<Type> Self, typename Env>
            requires(concepts::DecayConstructible<meta::Like<Self, Fun>>)
            friend auto tag_invoke(types::Tag<get_completion_signatures>, Self&&, Env&&)
                -> Signatures<meta::Like<Self, Seq>, Fun, MakeEnv<Env>>;

            friend auto tag_invoke(types::Tag<get_env>, Type const& self) { return make_env(get_env(self.sequence)); }
        };
    };

    template<concepts::Sender Seq, concepts::MovableValue Fun>
    using Sequence = meta::Type<SequenceT<meta::RemoveCVRef<Seq>, meta::Decay<Fun>>>;

    struct Function {
        template<concepts::Sender Seq, concepts::MovableValue Fun>
        concepts::SequenceSender auto operator()(Seq&& sequence, Fun&& transformer) const {
            if constexpr (concepts::TagInvocable<Function, Seq, Fun>) {
                static_assert(concepts::SequenceSender<meta::InvokeResult<Function, Seq, Fun>>,
                              "The return type of the transform_each function must be a sequence sender.");
                return function::tag_invoke(*this, util::forward<Seq>(sequence), util::forward<Fun>(transformer));
            } else {
                return Sequence<Seq, Fun> { util::forward<Seq>(sequence), util::forward<Fun>(transformer) };
            }
        }
    };
}

/// @brief Transform each sender of a sequence.
///
/// @param sequence The sequence.
/// @param transformer The function to transform each sender of the sequence.
///
/// @return The transformed sequence.
///
/// @note The transformer function must be lvalue callable for each sender of the sequence, and return a sender.
/// Additionally, the completion signatures of the return sender must only depend on the completion signatures of the
/// input sender, as the determination of completion signatures is done without any knowledge of the exact type of the
/// sequence's senders.
///
/// @warning If the underlying sequence is not always lock-step, the transformer function must be thread-safe. If using
/// a non-thread-safe transformer function, first call execution::into_lockstep_sequence() on the sequence.
constexpr inline auto transform_each = function::curry_back(transform_each_ns::Function {}, meta::c_<2zu>);
}

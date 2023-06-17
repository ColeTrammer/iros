#pragma once

#include <di/concepts/decays_to.h>
#include <di/concepts/movable_value.h>
#include <di/execution/algorithm/schedule_from.h>
#include <di/execution/concepts/prelude.h>
#include <di/execution/concepts/receiver.h>
#include <di/execution/meta/prelude.h>
#include <di/execution/receiver/receiver_adaptor.h>
#include <di/execution/receiver/set_value.h>
#include <di/execution/types/prelude.h>
#include <di/function/curry_back.h>
#include <di/util/defer_construct.h>

namespace di::execution {
namespace repeat_effect_until_ns {
    template<typename Send, typename Rec, typename Pred>
    struct OperationStateT {
        struct Type;
    };

    template<concepts::Sender Send, concepts::Receiver Rec, concepts::MovableValue Pred>
    using OperationState = meta::Type<OperationStateT<Send, Rec, Pred>>;

    template<typename Send, typename Rec, typename Pred>
    struct ReceiverT {
        struct Type : ReceiverAdaptor<Type> {
            using Base = ReceiverAdaptor<Type>;
            friend Base;

        public:
            explicit Type(OperationState<Send, Rec, Pred>* data) : m_op_state(data) {}

            Rec const& base() const& { return m_op_state->m_receiver; }
            Rec&& base() && { return util::move(m_op_state->m_receiver); }

        private:
            void set_value() && { m_op_state->repeat_effect(); }

            OperationState<Send, Rec, Pred>* m_op_state;
        };
    };

    template<concepts::Sender Send, concepts::Receiver Rec, concepts::MovableValue Pred>
    using Receiver = meta::Type<ReceiverT<Send, Rec, Pred>>;

    template<typename Send, typename Rec, typename Pred>
    struct OperationStateT<Send, Rec, Pred>::Type : util::Immovable {
    private:
        friend Receiver<Send, Rec, Pred>;

    public:
        template<typename S>
        explicit Type(Pred predicate, Rec receiver, S&& sender)
            : m_predicate(util::move(predicate)), m_sender(util::forward<S>(sender)), m_receiver(util::move(receiver)) {
            reconnect();
        }

    private:
        void reconnect() {
            m_op_state2.emplace(util::DeferConstruct([&] {
                return execution::connect(util::as_const(m_sender), Receiver<Send, Rec, Pred>(this));
            }));
        }

        void repeat_effect() {
            if (function::invoke(m_predicate)) {
                execution::set_value(util::move(m_receiver));
            } else {
                reconnect();
                restart();
            }
        }

        void restart() { execution::start(*m_op_state2); }

        friend void tag_invoke(types::Tag<execution::start>, Type& self) { self.restart(); }

        [[no_unique_address]] Pred m_predicate;
        [[no_unique_address]] Send m_sender;
        [[no_unique_address]] Rec m_receiver;
        [[no_unique_address]] Optional<meta::ConnectResult<Send const&, Receiver<Send, Rec, Pred>>> m_op_state2;
    };

    template<typename Send, typename Pred>
    struct SenderT {
        struct Type {
            using is_sender = void;

            [[no_unique_address]] Send sender;
            [[no_unique_address]] Pred predicate;

        private:
            template<concepts::DecaysTo<Type> Self, typename Rec>
            requires(concepts::DecayConstructible<Send const&> &&
                     concepts::SenderTo<Send const&, Receiver<Send, Rec, Pred>>)
            friend auto tag_invoke(types::Tag<connect>, Self&& self, Rec receiver) {
                return OperationState<Send, Rec, Pred> { util::forward<Self>(self).predicate, util::move(receiver),
                                                         util::forward<Self>(self).sender };
            }

            template<concepts::DecaysTo<Type> Self, typename Env>
            friend auto tag_invoke(types::Tag<get_completion_signatures>, Self&&, Env&&)
                -> meta::MakeCompletionSignatures<Send const&, MakeEnv<Env>>;

            constexpr friend auto tag_invoke(types::Tag<get_env>, Type const& self) {
                return make_env(get_env(self.sender));
            }
        };
    };

    template<concepts::Sender Send, concepts::MovableValue Pred>
    using Sender = meta::Type<SenderT<Send, Pred>>;

    struct Function {
        template<concepts::SenderOf<SetValue()> Send, concepts::MovableValue Pred>
        requires(concepts::CopyConstructible<Send> && concepts::Predicate<meta::Decay<Pred>&>)
        constexpr concepts::SenderOf<SetValue()> auto operator()(Send&& sender, Pred&& predicate) const {
            if constexpr (concepts::TagInvocable<Function, Send, Pred>) {
                return function::tag_invoke(*this, util::forward<Send>(sender), util::forward<Pred>(predicate));
            } else {
                return Sender<meta::Decay<Send>, meta::Decay<Pred>> { util::forward<Send>(sender),
                                                                      util::forward<Pred>(predicate) };
            }
        }
    };
}

constexpr inline auto repeat_effect_until = function::curry_back(repeat_effect_until_ns::Function {}, meta::c_<2zu>);
}

#pragma once

#include <di/execution/concepts/prelude.h>
#include <di/execution/interface/prelude.h>
#include <di/execution/meta/prelude.h>
#include <di/execution/query/prelude.h>
#include <di/execution/receiver/prelude.h>
#include <di/execution/types/prelude.h>
#include <di/util/defer_construct.h>

namespace di::execution {
namespace on_ns {
    template<typename Send, typename Rec, typename Sched>
    struct OperationStateT {
        struct Type;
    };

    template<concepts::Sender Send, concepts::Receiver Rec, concepts::Scheduler Sched>
    using OperationState = meta::Type<OperationStateT<Send, Rec, Sched>>;

    template<typename Base, typename Sched>
    struct EnvT {
        struct Type {
            [[no_unique_address]] Base base;
            [[no_unique_address]] Sched scheduler;

        private:
            constexpr friend auto tag_invoke(types::Tag<get_scheduler>, Type const& self) { return self.scheduler; }

            template<concepts::ForwardingQuery Tag>
            requires(!concepts::SameAs<Tag, types::Tag<get_scheduler>>)
            constexpr friend auto tag_invoke(types::Tag<get_env> tag, Type const& self)
                -> meta::InvokeResult<Tag, Base const&> {
                return tag(self.base);
            }
        };
    };

    template<typename Base, concepts::Scheduler Sched>
    using Env = meta::Type<EnvT<Base, Sched>>;

    template<typename Send, typename Rec, typename Sched>
    struct ReceiverWithEnvT {
        struct Type : private ReceiverAdaptor<Type> {
        private:
            using Base = ReceiverAdaptor<Type>;
            friend Base;

        public:
            explicit Type(OperationState<Send, Rec, Sched>* operation_state) : m_operation_state(operation_state) {}

            Rec const& base() const& { return m_operation_state->receiver; }
            Rec&& base() && { return util::move(m_operation_state->receiver); }

        private:
            Env<meta::EnvOf<Rec>, Sched> get_env() const {
                return { execution::get_env(base()), m_operation_state->scheduler };
            }

            OperationState<Send, Rec, Sched>* m_operation_state;
        };
    };

    template<concepts::Sender Send, concepts::Receiver Rec, concepts::Scheduler Sched>
    using ReceiverWithEnv = meta::Type<ReceiverWithEnvT<Send, Rec, Sched>>;

    template<typename Send, typename Rec, typename Sched>
    struct ReceiverT {
        struct Type : private ReceiverAdaptor<Type> {
        private:
            using Base = ReceiverAdaptor<Type>;
            friend Base;

        public:
            explicit Type(OperationState<Send, Rec, Sched>* operation_state) : m_operation_state(operation_state) {}

            Rec const& base() const& { return m_operation_state->receiver; }
            Rec&& base() && { return util::move(m_operation_state->receiver); }

        private:
            void set_value() && { m_operation_state->phase2(); }

            OperationState<Send, Rec, Sched>* m_operation_state;
        };
    };

    template<concepts::Sender Send, concepts::Receiver Rec, concepts::Scheduler Sched>
    using Receiver = meta::Type<ReceiverT<Send, Rec, Sched>>;

    template<typename Send, typename Rec, typename Sched>
    struct OperationStateT<Send, Rec, Sched>::Type {
    public:
        [[no_unique_address]] Sched scheduler;
        [[no_unique_address]] Send sender;
        [[no_unique_address]] Rec receiver;
        [[no_unique_address]] Variant<meta::ConnectResult<meta::ScheduleResult<Sched>, Receiver<Send, Rec, Sched>>,
                                      meta::ConnectResult<Send, ReceiverWithEnv<Send, Rec, Sched>>>
            operation_state;

        template<typename S>
        requires(concepts::ConstructibleFrom<Send, S>)
        explicit Type(Sched scheduler_, S&& sender_, Rec receiver_)
            : scheduler(util::move(scheduler_))
            , sender(util::move(sender_))
            , receiver(util::move(receiver_))
            , operation_state(in_place_index<0>, util::DeferConstruct([&] {
                                  return execution::connect(execution::schedule(scheduler),
                                                            Receiver<Send, Rec, Sched> { this });
                              })) {}

        void phase2() {
            operation_state.template emplace<1>(util::DeferConstruct([&] {
                return execution::connect(util::move(sender), ReceiverWithEnv<Send, Rec, Sched> { this });
            }));
            execution::start(util::get<1>(operation_state));
        }

    private:
        friend void tag_invoke(types::Tag<execution::start>, Type& self) {
            execution::start(util::get<0>(self.operation_state));
        }
    };

    template<typename Send, typename Sched>
    struct SenderT {
        struct Type {
            [[no_unique_address]] Sched scheduler;
            [[no_unique_address]] Send sender;

        private:
            template<concepts::DecaysTo<Type> Self, typename Rec>
            requires(concepts::DecayConstructible<meta::Like<Self, Send>> &&
                     concepts::SenderTo<meta::Like<Self, Send>, ReceiverWithEnv<Send, Rec, Sched>>)
            friend auto tag_invoke(types::Tag<connect>, Self&& self, Rec receiver) {
                return OperationState<Send, Rec, Sched> { util::forward<Self>(self).scheduler,
                                                          util::forward<Self>(self).sender, util::move(receiver) };
            }

            template<concepts::DecaysTo<Type> Self, typename E>
            friend auto tag_invoke(types::Tag<get_completion_signatures>, Self&&, E) -> meta::MakeCompletionSignatures<
                meta::Like<Self, Send>, Env<E, Sched>,
                meta::MakeCompletionSignatures<meta::ScheduleResult<Sched>, E, CompletionSignatures<>,
                                               meta::Id<CompletionSignatures<>>::template Invoke>>;

            template<concepts::ForwardingQuery Tag, typename... Args>
            constexpr friend auto tag_invoke(Tag tag, Type const& self, Args&&... args)
                -> meta::InvokeResult<Tag, Send const&, Args...> {
                return tag(self.sender, util::forward<Args>(args)...);
            }
        };
    };

    template<concepts::Sender Send, concepts::Scheduler Sched>
    using Sender = meta::Type<SenderT<Send, Sched>>;

    struct Function {
        template<concepts::Scheduler Sched, concepts::Sender Send>
        concepts::Sender auto operator()(Sched&& scheduler, Send&& sender) const {
            if constexpr (concepts::TagInvocable<Function, Sched, Send>) {
                return function::tag_invoke(*this, util::forward<Sched>(scheduler), util::forward<Send>(sender));
            } else {
                return Sender<meta::Decay<Send>, meta::Decay<Sched>> { util::forward<Sched>(scheduler),
                                                                       util::forward<Send>(sender) };
            }
        }
    };
}

/// execution::on() takes a scheduler and sender, and returns a new sender
/// whose which "runs" the provided sender on designated scheduler.
///
/// This is implemented by "connect"ing to the result of execution::schedule(),
/// and only starting the provided sender when that completes. Additionally,
/// execution::on() wraps any provided receivers with a new enviornment which
/// adverties the scheduler for execution::get_scheduler().
constexpr inline auto on = on_ns::Function {};
}

#pragma once

#include <di/execution/concepts/prelude.h>
#include <di/execution/interface/get_env.h>
#include <di/execution/meta/env_of.h>
#include <di/execution/meta/prelude.h>
#include <di/execution/receiver/prelude.h>
#include <di/execution/types/prelude.h>
#include <di/function/tag_invoke.h>
#include <di/util/defer_construct.h>

namespace di::execution {
namespace schedule_from_ns {
    template<typename Rec, typename Sched, typename Completions>
    struct ScheduleReceiverT {
        struct Type;
    };

    template<concepts::Receiver Rec, concepts::Scheduler Sched, concepts::InstanceOf<CompletionSignatures> Completions>
    using ScheduleReceiver = meta::Type<ScheduleReceiverT<Rec, Sched, Completions>>;

    template<typename Rec, typename Sched, typename Completions>
    struct DataT {
        struct Type {
        private:
            using List = meta::AsList<Completions>;
            using Tags =
                meta::Transform<List,
                                meta::Compose<meta::Quote<meta::List>, meta::Quote<meta::LanguageFunctionReturn>>>;
            using Args = meta::Transform<List, meta::Quote<meta::AsList>>;
            using Combined = meta::Transform<meta::Zip<Tags, Args>, meta::Quote<meta::Join>>;
            using Tupls = meta::Transform<Combined, meta::Uncurry<meta::Quote<meta::DecayedTuple>>>;
            using ArgsStorage = meta::AsTemplate<meta::VariantOrEmpty, meta::PushFront<Tupls, Tuple<Void>>>;

            using OpState3 =
                meta::ConnectResult<meta::ScheduleResult<Sched>, ScheduleReceiver<Rec, Sched, Completions>>;

        public:
            explicit Type(Sched sch_, Rec out_r_) : sch(util::move(sch_)), out_r(util::move(out_r_)) {}

            [[no_unique_address]] Sched sch;
            [[no_unique_address]] Rec out_r;
            [[no_unique_address]] ArgsStorage args {};
            [[no_unique_address]] Optional<OpState3> op_state3 {};

            template<concepts::OneOf<SetValue, SetError, SetStopped> Tag, concepts::DecayConstructible... Args,
                     typename Tup = meta::DecayedTuple<Tag, Args...>>
            requires(meta::Contains<meta::AsList<ArgsStorage>, Tup> &&
                     concepts::ReceiverOf<Rec, CompletionSignatures<Tag(meta::Decay<Args>...)>>)
            void phase2(Tag tag, Args&&... values) {
                args.template emplace<Tup>(tag, util::forward<Args>(values)...);

                auto& op_state = op_state3.emplace(util::DeferConstruct([&] {
                    return execution::connect(execution::schedule(sch),
                                              ScheduleReceiver<Rec, Sched, Completions> { this });
                }));

                execution::start(op_state);
            }
        };
    };

    template<concepts::Receiver Rec, concepts::Scheduler Sched, concepts::InstanceOf<CompletionSignatures> Completions>
    using Data = meta::Type<DataT<Rec, Sched, Completions>>;

    template<typename Rec, typename Sched, typename Completions>
    struct ScheduleReceiverT<Rec, Sched, Completions>::Type : ReceiverAdaptor<Type> {
        using Base = ReceiverAdaptor<Type>;
        friend Base;

    public:
        explicit Type(Data<Rec, Sched, Completions>* data) : m_data(data) {}

        auto base() const& -> Rec const& { return m_data->out_r; }
        auto base() && -> Rec&& { return util::move(m_data->out_r); }

    private:
        void set_value() && {
            visit(
                [&]<typename T>(T&& value) {
                    apply(
                        [&]<typename... Args>(auto tag, Args&&... args) {
                            if constexpr (concepts::SameAs<decltype(tag), Void>) {
                                DI_ASSERT(false);
                            } else {
                                tag(util::move(m_data->out_r), util::forward<Args>(args)...);
                            }
                        },
                        util::forward<T>(value));
                },
                util::move(m_data->args));
        }

        Data<Rec, Sched, Completions>* m_data;
    };

    template<typename Rec, typename Sched, typename Completions>
    struct ReceiverT {
        struct Type : ReceiverAdaptor<Type> {
            using Base = ReceiverAdaptor<Type>;
            friend Base;

        public:
            explicit Type(Data<Rec, Sched, Completions>* data) : m_data(data) {}

            auto base() const& -> Rec const& { return m_data->out_r; }
            auto base() && -> Rec&& { return util::move(m_data->out_r); }

        private:
            template<typename... Args>
            requires(requires {
                util::declval<Data<Rec, Sched, Completions>&>().phase2(SetValue {}, util::declval<Args>()...);
            })
            void set_value(Args&&... args) && {
                return m_data->phase2(SetValue {}, util::forward<Args>(args)...);
            }

            template<typename Error>
            requires(requires {
                util::declval<Data<Rec, Sched, Completions>&>().phase2(SetError {}, util::declval<Error>());
            })
            void set_error(Error&& error) && {
                return m_data->phase2(SetError {}, util::forward<Error>(error));
            }

            void set_stopped() && requires(requires {
                util::declval<Data<Rec, Sched, Completions>&>().phase2(SetStopped {});
            }) { return m_data->phase2(SetStopped {}); }

                Data<Rec, Sched, Completions>* m_data;
        };
    };

    template<concepts::Receiver Rec, concepts::Scheduler Sched, concepts::InstanceOf<CompletionSignatures> Completions>
    using Receiver = meta::Type<ReceiverT<Rec, Sched, Completions>>;

    template<typename Send, typename Rec, typename Sched>
    struct OperationStateT {
        struct Type : util::Immovable {
        private:
            using Completions = meta::CompletionSignaturesOf<Send, meta::EnvOf<Rec>>;

            using OpState2 = meta::ConnectResult<Send, Receiver<Rec, Sched, Completions>>;

        public:
            template<typename S>
            explicit Type(Sched scheduler, Rec receiver, S&& sender)
                : m_data(util::move(scheduler), util::move(receiver))
                , m_op_state2(execution::connect(util::forward<S>(sender),
                                                 Receiver<Rec, Sched, Completions> { util::addressof(m_data) })) {}

        private:
            friend void tag_invoke(types::Tag<execution::start>, Type& self) { execution::start(self.m_op_state2); }

            [[no_unique_address]] Data<Rec, Sched, Completions> m_data;
            DI_IMMOVABLE_NO_UNIQUE_ADDRESS OpState2 m_op_state2;
        };
    };

    template<concepts::Sender Send, concepts::Receiver Rec, concepts::Scheduler Sched>
    using OperationState = meta::Type<OperationStateT<Send, Rec, Sched>>;

    template<typename Send, typename Sched>
    struct SenderT {
        struct Type {
        public:
            using is_sender = void;

            [[no_unique_address]] Send sender;
            [[no_unique_address]] Sched scheduler;

        private:
            template<concepts::DecaysTo<Type> Self, typename Env>
            friend auto tag_invoke(types::Tag<get_completion_signatures>, Self&&, Env&&)
                -> meta::MakeCompletionSignatures<
                    meta::Like<Self, Send>, MakeEnv<Env>,
                    meta::MakeCompletionSignatures<meta::ScheduleResult<Sched>, MakeEnv<Env>, CompletionSignatures<>,
                                                   meta::TypeConstant<CompletionSignatures<>>::template Invoke>> {
                return {};
            }

            template<concepts::DecaysTo<Type> Self, typename Rec>
            requires(concepts::DecayConstructible<meta::Like<Self, Send>> &&
                     concepts::SenderTo<
                         meta::Like<Self, Send>,
                         Receiver<Rec, Sched,
                                  meta::CompletionSignaturesOf<meta::Like<Self, Send>, MakeEnv<meta::EnvOf<Rec>>>>>)
            friend auto tag_invoke(types::Tag<connect>, Self&& self, Rec receiver) {
                return OperationState<Send, Rec, Sched> { util::forward<Self>(self).scheduler, util::move(receiver),
                                                          util::forward<Self>(self).sender };
            }

            using SenderEnv = meta::EnvOf<Send>;

            struct Env {
                Sched scheduler;
                SenderEnv sender_env;

                template<concepts::OneOf<GetCompletionScheduler<SetValue>, GetCompletionScheduler<SetStopped>> Tag>
                friend auto tag_invoke(Tag, Env const& self) {
                    return self.scheduler;
                }

                template<concepts::ForwardingQuery Tag, typename... Args>
                requires(!concepts::OneOf<Tag, GetCompletionScheduler<SetValue>, GetCompletionScheduler<SetError>,
                                          GetCompletionScheduler<SetStopped>>)
                constexpr friend auto tag_invoke(Tag tag, Env const& self, Args&&... args)
                    -> meta::InvokeResult<Tag, SenderEnv const&, Args...> {
                    return tag(self.sender_env, util::forward<Args>(args)...);
                }
            };

            friend auto tag_invoke(types::Tag<get_env>, Type const& self) {
                return Env { self.scheduler, get_env(self.sender) };
            }
        };
    };

    template<concepts::Sender Send, concepts::Scheduler Sched>
    using Sender = meta::Type<SenderT<Send, Sched>>;

    struct Function {
        template<concepts::Scheduler Sched, concepts::Sender Send>
        auto operator()(Sched&& scheduler, Send&& sender) const -> concepts::Sender auto {
            if constexpr (concepts::TagInvocable<Function, Sched, Send>) {
                return function::tag_invoke(*this, util::forward<Sched>(scheduler), util::forward<Send>(sender));
            } else {
                return Sender<meta::Decay<Send>, meta::Decay<Sched>> { util::forward<Send>(sender),
                                                                       util::forward<Sched>(scheduler) };
            }
        }
    };
}

constexpr inline auto schedule_from = schedule_from_ns::Function {};
}

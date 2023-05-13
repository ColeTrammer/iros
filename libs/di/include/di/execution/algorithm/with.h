#pragma once

#include <di/concepts/movable_value.h>
#include <di/execution/concepts/prelude.h>
#include <di/execution/meta/prelude.h>
#include <di/execution/receiver/prelude.h>
#include <di/execution/types/empty_env.h>
#include <di/execution/types/prelude.h>
#include <di/util/defer_construct.h>

namespace di::execution {
namespace with_ns {
    template<typename Rec, typename Fun, typename Value, typename Completions>
    struct DestroyReceiverT {
        struct Type;
    };

    template<concepts::Receiver Rec, concepts::MovableValue Fun, concepts::AsyncDestroyable Value,
             concepts::InstanceOf<CompletionSignatures> Completions>
    using DestroyReceiver = meta::Type<DestroyReceiverT<Rec, Fun, Value, Completions>>;

    template<typename Rec, typename Fun, typename Value, typename Completions>
    struct ReceiverT {
        struct Type;
    };

    template<concepts::Receiver Rec, concepts::MovableValue Fun, concepts::AsyncDestroyable Value,
             concepts::InstanceOf<CompletionSignatures> Completions>
    using Receiver = meta::Type<ReceiverT<Rec, Fun, Value, Completions>>;

    template<typename Rec, typename Fun, typename Value, typename Completions>
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

            using Sender2 = meta::InvokeResult<Fun&, Value&>;
            using OpState3 = meta::ConnectResult<Sender2, Receiver<Rec, Fun, Value, Completions>>;
            using OpState4 =
                meta::ConnectResult<meta::AsyncDestroyResult<Value>, DestroyReceiver<Rec, Fun, Value, Completions>>;

        public:
            explicit Type(Fun function_, Rec out_r_) : function(util::move(function_)), out_r(util::move(out_r_)) {}

            [[no_unique_address]] Fun function;
            [[no_unique_address]] Rec out_r;
            [[no_unique_address]] Optional<Value> value;
            [[no_unique_address]] Optional<OpState3> op_state3;
            [[no_unique_address]] ArgsStorage args {};
            [[no_unique_address]] Optional<OpState4> op_state4 {};

            template<concepts::OneOf<SetValue, SetError, SetStopped> Tag, concepts::DecayConstructible... Args,
                     typename Tup = meta::DecayedTuple<Tag, Args...>>
            requires(meta::Contains<meta::AsList<ArgsStorage>, Tup> &&
                     concepts::ReceiverOf<Rec, CompletionSignatures<Tag(meta::Decay<Args>...)>>)
            void phase2(Tag tag, Args&&... values) {
                args.template emplace<Tup>(tag, util::forward<Args>(values)...);

                auto& op_state = op_state4.emplace(util::DeferConstruct([&] {
                    return execution::connect(execution::async_destroy<Value>(*value),
                                              DestroyReceiver<Rec, Fun, Value, Completions> { this });
                }));

                execution::start(op_state);
            }

            void phase1(Value&& value) {
                auto& v = this->value.emplace(util::move(value));

                auto& op_state = op_state3.template emplace(util::DeferConstruct([&] {
                    return execution::connect(function::invoke(function, v),
                                              Receiver<Rec, Fun, Value, Completions> { this });
                }));

                execution::start(op_state);
            }
        };
    };

    template<concepts::Receiver Rec, concepts::MovableValue Fun, concepts::AsyncDestroyable Value,
             concepts::InstanceOf<CompletionSignatures> Completions>
    using Data = meta::Type<DataT<Rec, Fun, Value, Completions>>;

    template<typename Rec, typename Fun, typename Value, typename Completions>
    struct DestroyReceiverT<Rec, Fun, Value, Completions>::Type : ReceiverAdaptor<Type> {
        using Base = ReceiverAdaptor<Type>;
        friend Base;

    public:
        explicit Type(Data<Rec, Fun, Value, Completions>* data) : m_data(data) {}

        Rec const& base() const& { return m_data->out_r; }
        Rec&& base() && { return util::move(m_data->out_r); }

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

        Data<Rec, Fun, Value, Completions>* m_data;
    };

    template<typename Rec, typename Fun, typename Value, typename Completions>
    struct ReceiverT<Rec, Fun, Value, Completions>::Type : ReceiverAdaptor<Type> {
        using Base = ReceiverAdaptor<Type>;
        friend Base;

    public:
        explicit Type(Data<Rec, Fun, Value, Completions>* data) : m_data(data) {}

        Rec const& base() const& { return m_data->out_r; }
        Rec&& base() && { return util::move(m_data->out_r); }

    private:
        Data<Rec, Fun, Value, Completions>* m_data;

        template<typename... Args>
        void set_value(Args&&... args) && {
            return m_data->phase2(SetValue {}, util::forward<Args>(args)...);
        }

        template<typename Error>
        void set_error(Error&& error) && {
            return m_data->phase2(SetError {}, util::forward<Error>(error));
        }

        void set_stopped() && { return m_data->phase2(SetStopped {}); }
    };

    template<typename Rec, typename Fun, typename Value, typename Completions>
    struct CreateReceiverT {
        struct Type : ReceiverAdaptor<Type> {
            using Base = ReceiverAdaptor<Type>;
            friend Base;

        public:
            explicit Type(Data<Rec, Fun, Value, Completions>* data) : m_data(data) {}

            Rec const& base() const& { return m_data->out_r; }
            Rec&& base() && { return util::move(m_data->out_r); }

        private:
            Data<Rec, Fun, Value, Completions>* m_data;

            template<typename... Args>
                void set_value(Value&& value) &&
                requires(requires { m_data->phase1(util::move(value)); }) { return m_data->phase1(util::move(value)); }
        };
    };

    template<concepts::Receiver Rec, concepts::MovableValue Fun, concepts::AsyncDestroyable Value,
             concepts::InstanceOf<CompletionSignatures> Completions>
    using CreateReceiver = meta::Type<CreateReceiverT<Rec, Fun, Value, Completions>>;

    template<typename Send, typename Rec, typename Fun, typename Value>
    struct OperationStateT {
        struct Type {
        private:
            using Sender2 = meta::InvokeResult<Fun&, Value&>;
            using Completions = meta::CompletionSignaturesOf<Sender2, meta::EnvOf<Rec>>;

            using OpState2 = meta::ConnectResult<Send, CreateReceiver<Rec, Fun, Value, Completions>>;

        public:
            template<typename S>
            explicit Type(Fun function, Rec receiver, S&& sender)
                : m_data(util::move(function), util::move(receiver))
                , m_op_state2(
                      execution::connect(util::forward<S>(sender),
                                         CreateReceiver<Rec, Fun, Value, Completions> { util::addressof(m_data) })) {}

        private:
            Data<Rec, Fun, Value, Completions> m_data;
            OpState2 m_op_state2;

            friend void tag_invoke(types::Tag<execution::start>, Type& self) { execution::start(self.m_op_state2); }
        };
    };

    template<concepts::Sender Send, concepts::Receiver Rec, concepts::MovableValue Fun,
             concepts::AsyncDestroyable Value>
    using OperationState = meta::Type<OperationStateT<Send, Rec, Fun, Value>>;

    template<typename Send, typename Fun>
    struct SenderT {
        struct Type {
        private:
            using Value = meta::SingleSenderValueType<Send, types::EmptyEnv>;
            using Sender2 = meta::InvokeResult<Fun&, Value&>;
            using Sender3 = meta::AsyncDestroyResult<Value>;

        public:
            using is_sender = void;

            [[no_unique_address]] Send sender;
            [[no_unique_address]] Fun function;

        private:
            template<typename Env>
            friend auto tag_invoke(types::Tag<get_completion_signatures>, Type&&, Env)
                -> meta::MakeCompletionSignatures<
                    Send, Env,
                    meta::MakeCompletionSignatures<
                        Sender2, Env,
                        meta::MakeCompletionSignatures<Sender3, Env, CompletionSignatures<>,
                                                       meta::Id<CompletionSignatures<>>::template Invoke>>,
                    meta::Id<CompletionSignatures<>>::template Invoke>;

            template<typename Rec>
            requires(concepts::DecayConstructible<Send> &&
                     concepts::SenderTo<Send, CreateReceiver<Rec, Fun, Value,
                                                             meta::CompletionSignaturesOf<Sender2, meta::EnvOf<Rec>>>>)
            friend auto tag_invoke(types::Tag<connect>, Type&& self, Rec receiver) {
                return OperationState<Send, Rec, Fun, Value> { util::move(self).function, util::move(receiver),
                                                               util::move(self).sender };
            }

            constexpr friend decltype(auto) tag_invoke(types::Tag<get_env>, Type const& self) {
                return get_env(self.sender);
            }
        };
    };

    template<concepts::Sender Send, concepts::MovableValue Fun>
    using Sender = meta::Type<SenderT<Send, Fun>>;

    struct Function {
        template<concepts::SingleSender<types::EmptyEnv> Send, concepts::MovableValue Fun>
        requires(concepts::AsyncDestroyable<meta::SingleSenderValueType<Send, types::EmptyEnv>> &&
                 requires {
                     {
                         function::invoke(util::declval<meta::Decay<Fun>&>(),
                                          util::declval<meta::SingleSenderValueType<Send, types::EmptyEnv>&>())
                     } -> concepts::Sender;
                 })
        concepts::Sender auto operator()(Send&& sender, Fun&& function) const {
            if constexpr (requires {
                              function::tag_invoke(*this, get_completion_scheduler<SetValue>(get_env(sender)),
                                                   util::forward<Send>(sender), util::forward<Fun>(function));
                          }) {
                return function::tag_invoke(*this, get_completion_scheduler<SetValue>(get_env(sender)),
                                            util::forward<Send>(sender), util::forward<Fun>(function));
            } else if constexpr (requires {
                                     function::tag_invoke(*this, util::forward<Send>(sender),
                                                          util::forward<Fun>(function));
                                 }) {
                return function::tag_invoke(*this, util::forward<Send>(sender), util::forward<Fun>(function));
            } else {
                return Sender<Send, Fun> { util::forward<Send>(sender), util::forward<Fun>(function) };
            }
        }
    };
}

/// With takes a sender which creates an async destroyable object, and calls the provided function
/// with the result. The function returns a sender which is then executed (if the creation sender
/// was successful). Then, the result of the second sender is decay-copied into the operation state.
/// Then, the async destroy sender is run to destroy the created resource. When this completes, the
/// results of the second sender are forwarded to the out receiver.
constexpr inline auto with = function::curry_back(with_ns::Function {}, meta::size_constant<2>);
}

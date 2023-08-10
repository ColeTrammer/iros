#pragma once

#include <di/execution/algorithm/just.h>
#include <di/execution/meta/completion_signatures_of.h>
#include <di/execution/meta/connect_result.h>
#include <di/execution/meta/decayed_tuple.h>
#include <di/execution/meta/env_of.h>
#include <di/execution/meta/make_completion_signatures.h>
#include <di/execution/meta/value_types_of.h>
#include <di/execution/query/get_sequence_cardinality.h>
#include <di/execution/query/make_env.h>
#include <di/execution/receiver/receiver_adaptor.h>
#include <di/execution/receiver/set_error.h>
#include <di/execution/receiver/set_stopped.h>
#include <di/execution/receiver/set_value.h>
#include <di/execution/sequence/sequence_sender.h>
#include <di/execution/types/completion_signuatures.h>
#include <di/function/container/function.h>
#include <di/function/curry_back.h>
#include <di/function/invoke.h>
#include <di/function/overload.h>
#include <di/meta/algorithm.h>
#include <di/meta/core.h>
#include <di/meta/function.h>
#include <di/meta/language.h>
#include <di/util/addressof.h>
#include <di/util/declval.h>
#include <di/util/defer_construct.h>
#include <di/vocab/optional/optional_forward_declaration.h>
#include <di/vocab/tuple/apply.h>
#include <di/vocab/variant/visit.h>

namespace di::execution {
namespace filter_ns {
    template<typename T>
    using DecayedRValue = meta::Decay<T>;

    template<typename Env, typename... Sends>
    using NonValueCompletions = meta::AsTemplate<
        CompletionSignatures,
        meta::Unique<meta::PushBack<
            meta::Transform<meta::Concat<meta::ErrorTypesOf<Sends, Env, meta::List>...>,
                            meta::Compose<meta::BindFront<meta::Quote<meta::AsLanguageFunction>, SetError>,
                                          meta::Quote<meta::List>>>,
            SetStopped()>>>;

    template<typename Fun, typename Env>
    struct ValueSigs {
        template<typename... Values>
        using Invoke = NonValueCompletions<Env, meta::InvokeResult<Fun, Values&...>>;
    };

    template<typename Seq, typename Fun, typename Env>
    using ExtraSignatures =
        meta::MakeCompletionSignatures<dummy_ns::DummySenderOf<meta::CompletionSignaturesOf<Seq, Env>>, Env,
                                       CompletionSignatures<>, ValueSigs<Fun, Env>::template Invoke>;

    template<typename... Values>
    using DecayValues = CompletionSignatures<SetValue(DecayedRValue<Values>...)>;

    template<typename Seq, typename Fun, typename Env>
    using Signatures = meta::MakeCompletionSignatures<Seq, Env, ExtraSignatures<Seq, Fun, Env>, DecayValues>;

    template<concepts::LanguageFunction Sig>
    using SigAsTuple =
        meta::AsTemplate<meta::DecayedTuple, meta::PushFront<meta::AsList<Sig>, meta::LanguageFunctionReturn<Sig>>>;

    template<typename Fun, typename Rec>
    struct DataT {
        struct Type {
            [[no_unique_address]] Fun predicate;
            [[no_unique_address]] Rec receiver;
        };
    };

    template<concepts::MovableValue Fun, concepts::Receiver Rec>
    using Data = meta::Type<DataT<meta::Decay<Fun>, Rec>>;

    struct Empty {};

    template<typename... Types>
    using OptionalVariant = meta::AsTemplate<Variant, meta::Unique<meta::List<Empty, Types...>>>;

    template<typename Send, typename Fun, typename Rec, typename R>
    struct ItemDataT {
        struct Type {
            using Env = MakeEnv<meta::EnvOf<R>>;
            using Values = meta::Apply<
                meta::Quote<OptionalVariant>,
                meta::Transform<meta::AsList<meta::CompletionSignaturesOf<Send, Env>>, meta::Quote<SigAsTuple>>>;

            explicit Type(Data<Fun, Rec>* data, R receiver) : data(data), receiver(di::move(receiver)) {}

            [[no_unique_address]] Data<Fun, Rec>* data;
            [[no_unique_address]] R receiver;
            [[no_unique_address]] Values values;

            di::Function<void()> on_receive_value;
            di::Function<void(bool)> on_filter_result;
            di::Function<void(bool)> on_sent_value;
        };
    };

    template<typename Send, typename Fun, typename Rec, typename R>
    using ItemData = meta::Type<ItemDataT<Send, Fun, Rec, R>>;

    template<typename Send, typename Fun, typename Rec, typename R>
    struct ItemReceiverT {
        struct Type : ReceiverAdaptor<Type> {
            using Base = ReceiverAdaptor<Type>;
            friend Base;

            explicit Type(ItemData<Send, Fun, Rec, R>* data) : m_data(data) {}

            R const& base() const& { return m_data->receiver; }
            R&& base() && { return di::move(m_data->receiver); }

            template<typename... Values>
            void set_value(Values&&... values) && {
                using Tuple = meta::DecayedTuple<SetValue, Values...>;
                m_data->values.template emplace<Tuple>(SetValue {}, di::forward<Values>(values)...);

                m_data->on_receive_value();
            }

            template<typename Error>
            void set_error(Error&& error) && {
                using Tuple = meta::DecayedTuple<SetError, Error>;
                m_data->values.template emplace<Tuple>(SetError {}, di::forward<Error>(error));

                m_data->on_receive_value();
            }

            void set_stopped() && {
                using Tuple = meta::DecayedTuple<SetStopped>;
                m_data->values.template emplace<Tuple>(SetStopped {});

                m_data->on_receive_value();
            }

        private:
            ItemData<Send, Fun, Rec, R>* m_data;
        };
    };

    template<typename Send, typename Fun, typename Rec, typename R>
    using ItemReceiver = meta::Type<ItemReceiverT<Send, Fun, Rec, R>>;

    template<typename Send, typename Fun, typename Rec, typename R>
    struct IntermediateReceiverT {
        struct Type : ReceiverAdaptor<Type> {
        private:
            using Base = ReceiverAdaptor<Type>;
            friend Base;

        public:
            explicit Type(ItemData<Send, Fun, Rec, R>* data) : m_data(data) {}

            R const& base() const& { return m_data->receiver; }
            R&& base() && { return di::move(m_data->receiver); }

            void set_value(bool accept) && { m_data->on_filter_result(accept); }

        private:
            ItemData<Send, Fun, Rec, R>* m_data;
        };
    };

    template<typename Send, typename Fun, typename Rec, typename R>
    using IntermediateReceiver = meta::Type<IntermediateReceiverT<Send, Fun, Rec, R>>;

    template<typename Send, typename Fun, typename Rec, typename R>
    struct FinalReceiverT {
        struct Type : ReceiverAdaptor<Type> {
        private:
            using Base = ReceiverAdaptor<Type>;
            friend Base;

        public:
            explicit Type(ItemData<Send, Fun, Rec, R>* data) : m_data(data) {}

            R const& base() const& { return m_data->receiver; }
            R&& base() && { return di::move(m_data->receiver); }

            void set_value() && { m_data->on_sent_value(true); }
            void set_stopped() && { m_data->on_sent_value(false); }

        private:
            ItemData<Send, Fun, Rec, R>* m_data;
        };
    };

    template<typename Send, typename Fun, typename Rec, typename R>
    using FinalReceiver = meta::Type<FinalReceiverT<Send, Fun, Rec, R>>;

    struct JustVarFunction {
        template<typename... Values>
        auto operator()(SetValue, Values&&... values) const {
            return just(di::forward<Values>(values)...);
        }

        template<typename Error>
        auto operator()(SetError, Error&& error) const {
            return just_error(di::forward<Error>(error));
        }

        auto operator()(SetStopped) const { return just_stopped(); }
    };

    constexpr inline auto just_var = JustVarFunction {};

    template<typename... Types>
    using JustVar = decltype(just_var(declval<Types>()...));

    template<typename Send, typename Fun, typename Rec, typename R>
    struct ItemOperationStateT {
        struct Type : util::Immovable {
        public:
            using Data = filter_ns::Data<Fun, Rec>;
            using ItemData = filter_ns::ItemData<Send, Fun, Rec, R>;
            using ItemReceiver = filter_ns::ItemReceiver<Send, Fun, Rec, R>;
            using IntermediateReceiver = filter_ns::IntermediateReceiver<Send, Fun, Rec, R>;
            using FinalReceiver = filter_ns::FinalReceiver<Send, Fun, Rec, R>;

            using Op = meta::ConnectResult<Send, ItemReceiver>;
            using IntermediateOps = meta::AsTemplate<
                OptionalVariant,
                meta::Transform<
                    meta::ValueTypesOf<Send, MakeEnv<meta::EnvOf<R>>, meta::List, meta::List>,
                    meta::Chain<meta::BindBack<meta::Quote<meta::Transform>, meta::Quote<meta::AddLValueReference>>,
                                meta::BindFront<meta::Quote<meta::Apply>,
                                                meta::Chain<meta::BindFront<meta::Quote<meta::InvokeResult>, Fun&>,
                                                            meta::BindBack<meta::Quote<meta::ConnectResult>,
                                                                           IntermediateReceiver>>>>>>;
            using FinalOps = meta::AsTemplate<
                OptionalVariant,
                meta::Transform<
                    meta::AsList<meta::CompletionSignaturesOf<Send, MakeEnv<meta::EnvOf<R>>>>,
                    meta::Chain<
                        meta::Quote<SigAsTuple>, meta::Quote<meta::AsList>,
                        meta::BindFront<
                            meta::Quote<meta::Apply>,
                            meta::Chain<meta::Quote<JustVar>, meta::BindFront<meta::Quote<meta::NextSenderOf>, Rec>,
                                        meta::BindBack<meta::Quote<meta::ConnectResult>, FinalReceiver>>>>>>;

            explicit Type(Send&& sender, Data* data, R receiver)
                : m_data(data, di::move(receiver))
                , m_op(connect(di::forward<Send>(sender), ItemReceiver(util::addressof(m_data)))) {
                m_data.on_receive_value = [this] {
                    di::visit(
                        di::overload(
                            [](Empty) {},
                            [this](auto& values) {
                                di::apply(
                                    [this](auto tag, auto&... values) {
                                        if constexpr (concepts::SameAs<decltype(tag), SetValue>) {
                                            using IntermediateSend = decltype(m_data.data->predicate(values...));
                                            using IntermediateOp =
                                                meta::ConnectResult<IntermediateSend, IntermediateReceiver>;

                                            auto& op = m_intermediate_op.template emplace<IntermediateOp>(
                                                di::DeferConstruct([&] {
                                                    return connect(m_data.data->predicate(values...),
                                                                   IntermediateReceiver(util::addressof(m_data)));
                                                }));
                                            start(op);
                                        } else {
                                            using FinalSend =
                                                meta::NextSenderOf<Rec, decltype(just_var(tag, di::move(values)...))>;
                                            using FinalOp = meta::ConnectResult<FinalSend, FinalReceiver>;

                                            auto& op = m_final_op.template emplace<FinalOp>(di::DeferConstruct([&] {
                                                return connect(
                                                    set_next(m_data.data->receiver, just_var(tag, di::move(values)...)),
                                                    FinalReceiver(di::addressof(m_data)));
                                            }));
                                            start(op);
                                        }
                                    },
                                    values);
                            }),
                        m_data.values);
                };

                m_data.on_filter_result = [this](bool accept) {
                    if (accept) {
                        di::visit(di::overload(
                                      [](Empty) {},
                                      [this](auto&& values) {
                                          di::apply(
                                              [this](auto tag, auto&&... values) {
                                                  if constexpr (SameAs<SetValue, decltype(tag)>) {
                                                      using FinalSend =
                                                          meta::NextSenderOf<Rec, decltype(just(di::move(values)...))>;
                                                      using FinalOp = meta::ConnectResult<FinalSend, FinalReceiver>;

                                                      auto& op =
                                                          m_final_op.template emplace<FinalOp>(di::DeferConstruct([&] {
                                                              return connect(set_next(m_data.data->receiver,
                                                                                      just(di::move(values)...)),
                                                                             FinalReceiver(di::addressof(m_data)));
                                                          }));
                                                      start(op);
                                                  }
                                              },
                                              values);
                                      }),
                                  di::move(m_data.values));
                    } else {
                        execution::set_value(di::move(m_data.receiver));
                    }
                };

                m_data.on_sent_value = [this](bool success) {
                    if (success) {
                        execution::set_value(di::move(m_data.receiver));
                    } else {
                        execution::set_stopped(di::move(m_data.receiver));
                    }
                };
            }

        private:
            friend void tag_invoke(Tag<start>, Type& self) { start(self.m_op); }

            ItemData m_data;
            DI_IMMOVABLE_NO_UNIQUE_ADDRESS Op m_op;
            IntermediateOps m_intermediate_op;
            FinalOps m_final_op;
        };
    };

    template<typename Send, typename Fun, typename Rec, typename R>
    using ItemOperationState = meta::Type<ItemOperationStateT<Send, Fun, Rec, R>>;

    template<typename Send, typename Fun, typename Rec>
    struct ItemSenderT {
        struct Type {
            using is_sender = void;
            using CompletionSignatures = di::CompletionSignatures<SetValue(), SetStopped()>;

            [[no_unique_address]] Send sender;
            Data<Fun, Rec>* data;

            template<concepts::RemoveCVRefSameAs<Type> Self, typename R>
            requires(concepts::DecayConstructible<meta::Like<Self, Fun>> &&
                     concepts::ReceiverOf<R, CompletionSignatures>)
            friend auto tag_invoke(types::Tag<connect>, Self&& self, R receiver) {
                return ItemOperationState<meta::Like<Self, Send>, Fun, Rec, R>(di::forward<Self>(self).sender,
                                                                               self.data, di::move(receiver));
            }

            auto tag_invoke(Tag<get_env>, Type const& self) { return make_env(get_env(self.sender)); }
        };
    };

    template<typename Send, typename Fun, typename Rec>
    using ItemSender = meta::Type<ItemSenderT<meta::RemoveCVRef<Send>, meta::Decay<Fun>, Rec>>;

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
            Rec&& base() && { return di::move(m_data->receiver); }

            template<concepts::Sender Next>
            concepts::NextSender auto set_next(Next&& next) & {
                return ItemSender<Next, Fun, Rec> { di::forward<Next>(next), m_data };
            }

        private:
            Data<Fun, Rec>* m_data;
        };
    };

    template<concepts::MovableValue Fun, concepts::Receiver Rec>
    using Receiver = meta::Type<ReceiverT<meta::Decay<Fun>, Rec>>;

    template<typename Seq, typename Fun, typename Rec>
    struct OperationStateT {
        struct Type : util::Immovable {
        public:
            using Receiver = filter_ns::Receiver<Fun, Rec>;
            using Op = meta::SubscribeResult<Seq, Receiver>;
            using Data = filter_ns::Data<Fun, Rec>;

            explicit Type(Seq&& sequence, Fun&& predicate, Rec receiver)
                : m_data(di::forward<Fun>(predicate), di::move(receiver))
                , m_op(subscribe(di::forward<Seq>(sequence), Receiver(util::addressof(m_data)))) {}

        private:
            friend void tag_invoke(types::Tag<start>, Type& self) { start(self.m_op); }

            [[no_unique_address]] Data m_data;
            DI_IMMOVABLE_NO_UNIQUE_ADDRESS Op m_op;
        };
    };

    template<typename Seq, typename Fun, typename Rec>
    using OperationState = meta::Type<OperationStateT<Seq, Fun, Rec>>;

    template<typename Seq, typename Fun>
    struct SequenceT {
        struct Type {
            using is_sender = SequenceTag;

            [[no_unique_address]] Seq sequence;
            [[no_unique_address]] Fun predicate;

            template<concepts::RemoveCVRefSameAs<Type> Self, typename Rec>
            requires(concepts::DecayConstructible<meta::Like<Self, Fun>> &&
                     concepts::SubscriberOf<Rec, Signatures<meta::Like<Self, Seq>, Fun, MakeEnv<meta::EnvOf<Rec>>>>)
            friend auto tag_invoke(types::Tag<subscribe>, Self&& self, Rec receiver) {
                return OperationState<meta::Like<Self, Seq>, meta::Like<Self, Fun>, Rec>(
                    di::forward<Self>(self).sequence, di::forward<Self>(self).predicate, di::move(receiver));
            }

            template<concepts::RemoveCVRefSameAs<Type> Self, typename Env>
            requires(concepts::DecayConstructible<meta::Like<Self, Fun>>)
            friend auto tag_invoke(types::Tag<get_completion_signatures>, Self&&, Env&&)
                -> Signatures<meta::Like<Self, Seq>, Fun, MakeEnv<Env>>;

            friend auto tag_invoke(types::Tag<get_env>, Type const& self) {
                return make_env(get_env(self.sequence), without(get_sequence_cardinality));
            }
        };
    };

    template<concepts::Sender Seq, concepts::MovableValue Fun>
    using Sequence = meta::Type<SequenceT<meta::RemoveCVRef<Seq>, meta::Decay<Fun>>>;

    struct Function {
        template<concepts::Sender Seq, concepts::MovableValue Fun>
        concepts::SequenceSender auto operator()(Seq&& sequence, Fun&& predicate) const {
            if constexpr (concepts::TagInvocable<Function, Seq, Fun>) {
                static_assert(concepts::SequenceSender<meta::InvokeResult<Function, Seq, Fun>>,
                              "The return type of the filter function must be a sequence sender.");
                return function::tag_invoke(*this, di::forward<Seq>(sequence), di::forward<Fun>(predicate));
            } else {
                return Sequence<Seq, Fun> { di::forward<Seq>(sequence), di::forward<Fun>(predicate) };
            }
        }
    };
}

/// @brief Filter the values of a sequence.
///
/// @param sequence The sequence.
/// @param predicate The function to filter the values of the sequence.
///
/// @return The filtered sequence.
///
/// @note The predicate function must be lvalue callable for each set of values produced by the sequence, and return a
/// sender which yields a bool. If the sequence item is an error, the predicate function is not called. If the predicate
/// function returns an error, that error is propagated as the result of the sequence.
///
/// @warning If the underlying sequence is not always lock-step, the predicate function must be thread-safe. If using
/// a non-thread-safe predicate function, first call execution::into_lockstep_sequence() on the sequence.
constexpr inline auto filter = function::curry_back(filter_ns::Function {}, meta::c_<2zu>);
}

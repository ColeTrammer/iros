#pragma once

#include <di/execution/algorithm/just.h>
#include <di/execution/algorithm/when_all.h>
#include <di/execution/concepts/receiver_of.h>
#include <di/execution/concepts/sender.h>
#include <di/execution/concepts/sender_to.h>
#include <di/execution/interface/connect.h>
#include <di/execution/interface/get_env.h>
#include <di/execution/interface/start.h>
#include <di/execution/meta/connect_result.h>
#include <di/execution/meta/env_of.h>
#include <di/execution/query/is_always_lockstep_sequence.h>
#include <di/execution/query/make_env.h>
#include <di/execution/receiver/set_stopped.h>
#include <di/execution/receiver/set_value.h>
#include <di/execution/sequence/empty_sequence.h>
#include <di/execution/sequence/into_lockstep_sequence.h>
#include <di/execution/sequence/into_variant_each.h>
#include <di/execution/sequence/sequence_sender.h>
#include <di/execution/types/completion_signuatures.h>
#include <di/function/container/function.h>
#include <di/function/tag_invoke.h>
#include <di/meta/core.h>
#include <di/meta/util.h>
#include <di/platform/compiler.h>
#include <di/sync/memory_order.h>
#include <di/sync/stop_token/in_place_stop_source.h>
#include <di/sync/stop_token/in_place_stop_token.h>
#include <di/util/addressof.h>
#include <di/util/defer_construct.h>
#include <di/util/exchange.h>
#include <di/util/immovable.h>
#include <di/util/move.h>
#include <di/vocab/array/array.h>
#include <di/vocab/optional/optional_forward_declaration.h>
#include <di/vocab/tuple/tuple.h>

namespace di::execution {
namespace zip_ns {
    using when_all_ns::Env;
    using when_all_ns::ErrorStorage;
    using when_all_ns::never_sends_value;
    using when_all_ns::NonValueCompletions;
    using when_all_ns::NotError;
    using when_all_ns::Sigs;
    using when_all_ns::StopCallbackFunction;
    using when_all_ns::Stopped;
    using when_all_ns::ValidSenders;
    using when_all_ns::ValueCompletion;
    using when_all_ns::ValueStorage;

    template<typename Rec, typename... Seqs>
    struct OuterDataT {
        struct Type {
        public:
            using Env = meta::EnvOf<Rec>;
            using Value = ValueStorage<Env, Seqs...>;
            using Error = ErrorStorage<Env, Seqs...>;
            using Count = usize;
            using StopToken = meta::StopTokenOf<Env>;
            using StopCallback = StopToken::template CallbackType<StopCallbackFunction>;

            explicit Type(Rec outer_out_r_) : outer_out_r(util::move(outer_out_r_)) {}

            void did_complete_outer_stopped() { did_complete(true); }
            void did_complete_outer_value() { did_complete(false); }

            void did_complete(bool stopped) {
                // Reset inner counters and state. There can't be races here since all sequences are lockstep.
                inner_error.template emplace<NotError>();
                inner_failed.store(false, sync::MemoryOrder::Relaxed);
                inner_remaining.store(outer_remaining.load(sync::MemoryOrder::Relaxed), sync::MemoryOrder::Relaxed);

                // Complete inner receivers.
                for (auto& inner_callback : inner_complete_callbacks) {
                    auto callback = util::move(inner_callback);
                    inner_callback = nullptr;
                    if (callback) {
                        callback(stopped);
                    }
                }
            }

            template<usize index, typename... Types>
            void report_inner_value(Constexpr<index>, Types&&... values) {
                if constexpr (never_sends_value<Env, Seqs...>) {
                    ((void) values, ...);
                } else {
                    auto& optional = util::get<index>(inner_value);
                    optional.emplace(util::forward<Types>(values)...);
                }
                finish_one_inner();
            }

            template<typename E>
            void report_inner_error(E&& error) {
                auto old = inner_failed.exchange(true, sync::MemoryOrder::AcquireRelease);
                if (!old) {
                    // NOTE: Do not request stop here since inner errors can be reported multiple times.
                    this->inner_error.template emplace<meta::Decay<E>>(util::forward<E>(error));
                }
                finish_one_inner();
            }

            void report_inner_stop() {
                auto old = inner_failed.exchange(true, sync::MemoryOrder::AcquireRelease);
                if (!old) {
                    stop_source.request_stop();
                    inner_error.template emplace<Stopped>();
                }
                finish_one_inner();
            }

            void finish_one_inner() {
                auto old_value = inner_remaining.fetch_sub(1, sync::MemoryOrder::AcquireRelease);
                if (old_value == 1) {
                    inner_finish_callback();
                }
            }

            void report_outer_value() {
                report_inner_stop();

                finish_one_outer();
            }

            template<typename E>
            void report_outer_error(E&& error) {
                report_inner_stop();

                auto old = outer_failed.exchange(true, sync::MemoryOrder::AcquireRelease);
                if (!old) {
                    stop_source.request_stop();
                    this->outer_error.template emplace<meta::Decay<E>>(util::forward<E>(error));
                }
                finish_one_outer();
            }

            void report_outer_stop() {
                report_inner_stop();

                auto old = outer_failed.exchange(true, sync::MemoryOrder::AcquireRelease);
                if (!old) {
                    stop_source.request_stop();
                    outer_error.template emplace<Stopped>();
                }
                finish_one_outer();
            }

            void finish_one_outer() {
                auto old_value = outer_remaining.fetch_sub(1, sync::MemoryOrder::AcquireRelease);
                if (old_value == 1) {
                    // Reset the stop callback.
                    stop_callback.reset();

                    // Check if an error occurred. This does not need to be atomic because we're done and just used
                    // acquire release ordering.
                    if (!vocab::holds_alternative<NotError>(outer_error)) {
                        if (!vocab::holds_alternative<Stopped>(outer_error)) {
                            vocab::visit(
                                [&]<typename E>(E&& e) {
                                    if constexpr (!concepts::OneOf<meta::RemoveCVRef<E>, NotError, Stopped>) {
                                        set_error(util::move(outer_out_r), util::forward<E>(e));
                                    }
                                },
                                util::move(outer_error));
                        } else {
                            set_stopped(util::move(outer_out_r));
                        }
                    } else {
                        set_value(util::move(outer_out_r));
                    }
                }
            }

            auto at_least_one_sequence_stopped() const -> bool {
                // This can use relaxed memory ordering, since it's only checked after all sequences have either
                // completed or have sent their next value.
                return outer_remaining.load(sync::MemoryOrder::Relaxed) < sizeof...(Seqs);
            }

            [[no_unique_address]] Value inner_value;
            [[no_unique_address]] Error inner_error;
            sync::Atomic<Count> inner_remaining { sizeof...(Seqs) };
            sync::Atomic<bool> inner_failed { false };
            vocab::Array<function::Function<void(bool)>, sizeof...(Seqs)> inner_complete_callbacks;
            function::Function<void()> inner_finish_callback;

            [[no_unique_address]] Error outer_error;
            [[no_unique_address]] Rec outer_out_r;
            sync::Atomic<Count> outer_remaining { sizeof...(Seqs) };
            sync::Atomic<bool> outer_failed { false };

            sync::InPlaceStopSource stop_source;
            vocab::Optional<StopCallback> stop_callback;
        };
    };

    template<concepts::Receiver Rec, concepts::Sender... Sends>
    using OuterData = meta::Type<OuterDataT<Rec, Sends...>>;

    template<usize index, typename Data>
    struct InnerReceiverT {
        struct Type {
            using is_receiver = void;

            Data* data;

            template<typename... Types>
            friend void tag_invoke(Tag<set_value>, Type&& self, Types&&... values) {
                self.data->report_inner_value(c_<index>, util::forward<Types>(values)...);
            }

            template<typename E>
            friend void tag_invoke(Tag<set_error>, Type&& self, E&& error) {
                self.data->report_inner_error(util::forward<E>(error));
            }

            friend void tag_invoke(Tag<set_stopped>, Type&& self) { self.data->report_inner_stop(); }

            friend auto tag_invoke(Tag<get_env>, Type const& self) {
                return make_env(get_env(self.data->outer_out_r),
                                with(get_stop_token, self.data->stop_source.get_stop_token()));
            }
        };
    };

    template<usize index, typename Data>
    using InnerReceiver = meta::Type<InnerReceiverT<index, Data>>;

    template<usize index, typename Send, typename Data, typename R>
    struct InnerNextOperationStateT {
        struct Type : util::Immovable {
        public:
            using InnerRec = InnerReceiver<index, Data>;
            using Op = meta::ConnectResult<Send, InnerRec>;

            explicit Type(Data* data, Send&& sender, R receiver)
                : m_data(data)
                , m_receiver(util::move(receiver))
                , m_op(connect(util::forward<Send>(sender), InnerRec(data))) {}

        private:
            friend void tag_invoke(Tag<start>, Type& self) {
                // Type-erasure is used here because there is no way to know the receiver type R before the operation
                // exists. Ideally, we could store a Tuple<R...> in the data, and use it to directly complete the
                // receiver. Since R is not known up front, this cannot be done. At least, the type-erased function
                // won't require allocations, since we only capture a single pointer.
                self.m_data->inner_complete_callbacks[index] = [&self](bool stopped) {
                    if (stopped) {
                        set_stopped(util::move(self.m_receiver));
                    } else {
                        set_value(util::move(self.m_receiver));
                    }
                };

                start(self.m_op);
            }

            Data* m_data;
            [[no_unique_address]] R m_receiver;
            DI_IMMOVABLE_NO_UNIQUE_ADDRESS Op m_op;
        };
    };

    template<usize index, typename Send, typename Data, typename R>
    using InnerNextOperationState = meta::Type<InnerNextOperationStateT<index, Send, Data, R>>;

    template<usize index, typename Send, typename Data>
    struct InnerNextSenderT {
        struct Type {
            using is_sender = void;

            using CompletionSignatures = types::CompletionSignatures<SetValue(), SetStopped()>;

            Data* data;
            [[no_unique_address]] Send sender;

            template<concepts::ReceiverOf<CompletionSignatures> R>
            friend auto tag_invoke(Tag<connect>, Type&& self, R receiver) {
                return InnerNextOperationState<index, Send, Data, R>(self.data, util::move(self.sender),
                                                                     util::move(receiver));
            }

            friend auto tag_invoke(Tag<get_env>, Type const& self) { return make_env(get_env(self.sender)); }
        };
    };

    template<usize index, typename Send, typename Data>
    using InnerNextSender = meta::Type<InnerNextSenderT<index, meta::RemoveCVRef<Send>, Data>>;

    template<usize index, typename Seq, typename Data>
    struct ReceiverT {
        struct Type {
            using is_receiver = void;

            Data* data;

            friend void tag_invoke(Tag<set_value>, Type&& self) { self.data->report_outer_value(); }

            template<typename E>
            friend void tag_invoke(Tag<set_error>, Type&& self, E&& error) {
                self.data->report_outer_error(util::forward<E>(error));
            }

            friend void tag_invoke(Tag<set_stopped>, Type&& self) { self.data->report_outer_stop(); }

            using NextRec = InnerReceiver<index, Data>;

            template<concepts::SenderTo<NextRec> Send>
            friend auto tag_invoke(Tag<set_next>, Type& self, Send&& sender) {
                return InnerNextSender<index, Send, Data>(self.data, util::forward<Send>(sender));
            }

            friend auto tag_invoke(Tag<get_env>, Type const& self) {
                return make_env(get_env(self.data->outer_out_r),
                                with(get_stop_token, self.data->stop_source.get_stop_token()));
            }
        };
    };

    template<usize index, concepts::Sender Seq, typename Data>
    using Receiver = meta::Type<ReceiverT<index, Seq, Data>>;

    template<typename Rec, typename Indices, typename Seqs>
    struct NextReceiverT;

    template<typename Rec, usize... indices, typename... Seqs>
    struct NextReceiverT<Rec, meta::ListV<indices...>, meta::List<Seqs...>> {
        struct Type {
            using is_receiver = void;

            OuterData<Rec, Seqs...>* data;

            friend void tag_invoke(Tag<set_value>, Type&& self) { self.data->did_complete_outer_value(); }
            friend void tag_invoke(Tag<set_stopped>, Type&& self) { self.data->did_complete_outer_stopped(); }

            friend auto tag_invoke(Tag<get_env>, Type const& self) {
                return make_env(get_env(self.data->outer_out_r),
                                with(get_stop_token, self.data->stop_source.get_stop_token()));
            }
        };
    };

    template<typename Rec, typename Indices, typename Seqs>
    using NextReceiver = meta::Type<NextReceiverT<Rec, Indices, Seqs>>;

    template<typename Rec, typename Indices, typename Seqs, typename R>
    struct NextOperationStateT;

    template<typename Rec, usize... indices, typename... Seqs, typename R>
    struct NextOperationStateT<Rec, meta::ListV<indices...>, meta::List<Seqs...>, R> {
        struct Type : util::Immovable {
            explicit Type(OuterData<Rec, Seqs...>* data, R receiver) : m_data(data), m_receiver(util::move(receiver)) {}

            friend void tag_invoke(Tag<start>, Type& self) {
                // The sent values have already been stored in the data, so just complete immediately.
                auto& data = *self.m_data;
                if (!vocab::holds_alternative<NotError>(data.inner_error)) {
                    if (!vocab::holds_alternative<Stopped>(data.inner_error)) {
                        vocab::visit(
                            [&]<typename E>(E&& e) {
                                if constexpr (!concepts::OneOf<meta::RemoveCVRef<E>, NotError, Stopped>) {
                                    execution::set_error(util::move(self.m_receiver), util::forward<E>(e));
                                }
                            },
                            util::move(data.inner_error));
                    } else {
                        execution::set_stopped(util::move(self.m_receiver));
                    }
                } else {
                    if constexpr (!never_sends_value<meta::EnvOf<Rec>, Seqs...>) {
                        // Value is a Tuple<Optional<Vs...>, ...>, and we need to return all Vs's. Do this be
                        // concatenating all the tuples and then unpacking them.
                        auto lvalues = vocab::apply(
                            [](auto&... optionals) {
                                return vocab::tuple_cat(vocab::apply(vocab::tie, *optionals)...);
                            },
                            data.inner_value);
                        vocab::apply(
                            [&](auto&... values) {
                                execution::set_value(util::move(self.m_receiver), util::move(values)...);
                            },
                            lvalues);
                    } else {
                        util::unreachable();
                    }
                }
            }

        private:
            OuterData<Rec, Seqs...>* m_data;
            [[no_unique_address]] R m_receiver;
        };
    };

    template<typename Rec, typename Indices, typename Seqs, typename R>
    using NextOperationState = meta::Type<NextOperationStateT<Rec, Indices, Seqs, R>>;

    template<typename Rec, typename Indices, typename Seqs>
    struct NextSenderT;

    template<typename Rec, usize... indices, typename... Seqs>
    struct NextSenderT<Rec, meta::ListV<indices...>, meta::List<Seqs...>> {
        struct Type {
        public:
            using is_sender = void;

            using CompletionSignatures = Sigs<meta::EnvOf<Rec>, Seqs...>;

            OuterData<Rec, Seqs...>* data;

            template<concepts::ReceiverOf<CompletionSignatures> R>
            friend auto tag_invoke(Tag<connect>, Type&& self, R receiver) {
                return NextOperationState<Rec, meta::ListV<indices...>, meta::List<Seqs...>, R>(self.data,
                                                                                                util::move(receiver));
            }
        };
    };

    template<typename Rec, typename Indices, typename Seqs>
    using NextSender = meta::Type<NextSenderT<Rec, Indices, Seqs>>;

    template<typename Rec, typename Indices, typename Seqs>
    struct OperationStateT;

    template<typename Rec, usize... indices, typename... Seqs>
    struct OperationStateT<Rec, meta::ListV<indices...>, meta::List<Seqs...>> {
        struct Type : util::Immovable {
        public:
            using Data = OuterData<Rec, Seqs...>;
            using OpStates = vocab::Tuple<meta::SubscribeResult<Seqs, Receiver<indices, Seqs, Data>>...>;

            using ItemSend = NextSender<Rec, meta::ListV<indices...>, meta::List<Seqs...>>;
            using OutSend = meta::NextSenderOf<Rec, ItemSend>;
            using NextRec = NextReceiver<Rec, meta::ListV<indices...>, meta::List<Seqs...>>;
            using NextOp = meta::ConnectResult<OutSend, NextRec>;

            explicit Type(Rec out_r, Seqs&&... sequences)
                : m_data(util::move(out_r))
                , m_op_states(util::DeferConstruct([&] -> meta::SubscribeResult<Seqs, Receiver<indices, Seqs, Data>> {
                    return subscribe(util::forward<Seqs>(sequences),
                                     Receiver<indices, Seqs, Data> { util::addressof(m_data) });
                })...) {}

        private:
            friend void tag_invoke(Tag<start>, Type& self) {
                // Emplace construct stop callback.
                self.m_data.stop_callback.emplace(get_stop_token(get_env(self.m_data.outer_out_r)),
                                                  StopCallbackFunction { self.m_data.stop_source });

                // Check if stop requested:
                if (self.m_data.stop_source.stop_requested()) {
                    set_stopped(util::move(self.m_data.outer_out_r));
                    return;
                }

                // Setup callback for when all inner next senders have completed.
                self.m_data.inner_finish_callback = [&self] {
                    // If any of the sequences stopped, then their is nothing to send at all. Instead, we complete this
                    // round.
                    if (self.m_data.at_least_one_sequence_stopped()) {
                        return self.m_data.did_complete_outer_stopped();
                    }

                    self.m_next_op.emplace(util::DeferConstruct([&] {
                        return connect(set_next(self.m_data.outer_out_r, ItemSend(util::addressof(self.m_data))),
                                       NextRec(util::addressof(self.m_data)));
                    }));
                    start(*self.m_next_op);
                };

                // Call start on all operations.
                vocab::tuple_for_each(start, self.m_op_states);
            }

            [[no_unique_address]] Data m_data;
            [[no_unique_address]] OpStates m_op_states;
            DI_IMMOVABLE_NO_UNIQUE_ADDRESS vocab::Optional<NextOp> m_next_op;
        };
    };

    template<concepts::Receiver Rec, typename Indices, concepts::TypeList Seqs>
    using OperationState = meta::Type<OperationStateT<Rec, Indices, Seqs>>;

    template<typename... Seqs>
    struct SequenceT {
        struct Type {
        public:
            using is_sender = SequenceTag;

            template<typename... Ts>
            explicit Type(InPlace, Seqs&&... sequences) : m_sequences(util::forward<Seqs>(sequences)...) {}

        private:
            template<concepts::RemoveCVRefSameAs<Type> Self, typename E>
            requires(ValidSenders<Env<E>, meta::Like<Self, Seqs>...>)
            friend auto tag_invoke(Tag<get_completion_signatures>, Self&&, E&&) -> Sigs<E, meta::Like<Self, Seqs>...> {
                return {};
            }

            template<concepts::RemoveCVRefSameAs<Type> Self, concepts::Receiver Rec>
            requires(
                concepts::SubscriberOf<Rec, meta::CompletionSignaturesOf<meta::Like<Self, Type>, meta::EnvOf<Rec>>>)
            friend auto tag_invoke(Tag<subscribe>, Self&& self, Rec out_r) {
                return vocab::apply(
                    [&](auto&&... sequences) {
                        return OperationState<Rec, meta::MakeIndexSequence<sizeof...(Seqs)>,
                                              meta::List<meta::Like<Self, Seqs>...>> {
                            util::move(out_r), util::forward<decltype(sequences)>(sequences)...
                        };
                    },
                    util::forward<Self>(self).m_sequences);
            }

            friend auto tag_invoke(Tag<get_env>, Type const&) {
                return make_env(
                    empty_env, with(is_always_lockstep_sequence, c_<true>),
                    with(get_sequence_cardinality, c_<container::min({ meta::SequenceCardinality<Seqs>... })>));
            }

            vocab::Tuple<Seqs...> m_sequences;
        };
    };

    template<concepts::Sender... Seqs>
    using Sequence = meta::Type<SequenceT<meta::RemoveCVRef<Seqs>...>>;

    struct Function {
        template<concepts::Sender... Seqs>
        requires(sizeof...(Seqs) > 0)
        auto operator()(Seqs&&... sequences) const {
            if constexpr (concepts::TagInvocable<Function, Seqs...>) {
                static_assert(concepts::SequenceSender<meta::TagInvokeResult<Function, Seqs...>>,
                              "zip() customizations must return a sequence sender");
                return tag_invoke(*this, util::forward<Seqs>(sequences)...);
            } else {
                return Sequence<meta::IntoLockstepSequence<Seqs>...> {
                    in_place, into_lockstep_sequence(util::forward<Seqs>(sequences))...
                };
            }
        }
    };

    struct VariantFunction {
        template<concepts::Sender... Seqs>
        requires(sizeof...(Seqs) > 0)
        auto operator()(Seqs&&... sequences) const {
            if constexpr (concepts::TagInvocable<VariantFunction, Seqs...>) {
                static_assert(concepts::SequenceSender<meta::TagInvokeResult<VariantFunction, Seqs...>>,
                              "zip_with_variant() customizations must return a sequence sender");
                return tag_invoke(*this, util::forward<Seqs>(sequences)...);
            } else {
                return Function {}(into_variant_each(util::forward<Seqs>(sequences))...);
            }
        }

        auto operator()() const { return empty_sequence(); }
    };
}

/// @brief Zip multiple sequences together.
///
/// @param sequences The sequences to zip together.
///
/// @returns A sequence sender that sends each value of each sequence together.
///
/// This function is like view::zip() for ranges, but for sequences. Like when_all(), the senders are
/// required to only send a single type of values. If you need to send multiple types of values, use
/// zip_with_variant().
///
/// This function is used to combine multiple sequences together into a single sequence. For example:
///
/// @snippet{trimleft} tests/test_execution_sequence.cpp zip
///
/// The sequences passed to this function will first be converted to lockstep sequences using
/// into_lockstep_sequence(). As a result, the sequence returned by this function will be a lockstep
/// sequence.
///
/// @see zip_with_variant
/// @see view::zip
constexpr inline auto zip = zip_ns::Function {};

/// @brief Zip multiple sequences together, allowing them to send different types of values.
///
/// @param sequences The sequences to zip together.
///
/// @returns A sequence sender that sends each value of each sequence together.
///
/// This function is like view::zip() for ranges, but for sequences. Unlike zip(), the senders are allowed
/// to send different types of values. The values will be sent as a variant.
///
/// @see zip
/// @see view::zip
/// @see into_variant_each
constexpr inline auto zip_with_variant = zip_ns::VariantFunction {};
}

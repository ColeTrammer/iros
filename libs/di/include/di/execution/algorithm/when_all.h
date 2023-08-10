#pragma once

#include <di/execution/algorithm/into_variant.h>
#include <di/execution/algorithm/transfer.h>
#include <di/execution/concepts/forwarding_query.h>
#include <di/execution/concepts/receiver.h>
#include <di/execution/concepts/receiver_of.h>
#include <di/execution/concepts/scheduler.h>
#include <di/execution/concepts/sender.h>
#include <di/execution/concepts/sender_in.h>
#include <di/execution/interface/connect.h>
#include <di/execution/interface/get_env.h>
#include <di/execution/interface/start.h>
#include <di/execution/meta/completion_signatures_of.h>
#include <di/execution/meta/connect_result.h>
#include <di/execution/meta/decayed_tuple.h>
#include <di/execution/meta/env_of.h>
#include <di/execution/meta/error_types_of.h>
#include <di/execution/meta/sends_stopped.h>
#include <di/execution/meta/stop_token_of.h>
#include <di/execution/meta/value_types_of.h>
#include <di/execution/query/get_completion_signatures.h>
#include <di/execution/query/get_stop_token.h>
#include <di/execution/query/make_env.h>
#include <di/execution/receiver/set_error.h>
#include <di/execution/receiver/set_stopped.h>
#include <di/execution/receiver/set_value.h>
#include <di/execution/types/completion_signuatures.h>
#include <di/function/invoke.h>
#include <di/function/tag_invoke.h>
#include <di/math/smallest_unsigned_type.h>
#include <di/meta/algorithm.h>
#include <di/meta/constexpr.h>
#include <di/meta/core.h>
#include <di/meta/util.h>
#include <di/sync/atomic.h>
#include <di/sync/memory_order.h>
#include <di/sync/stop_token/in_place_stop_token.h>
#include <di/sync/stop_token/prelude.h>
#include <di/types/integers.h>
#include <di/types/prelude.h>
#include <di/util/addressof.h>
#include <di/util/declval.h>
#include <di/util/defer_construct.h>
#include <di/util/get.h>
#include <di/util/immovable.h>
#include <di/util/move.h>
#include <di/util/unreachable.h>
#include <di/vocab/optional/optional_forward_declaration.h>
#include <di/vocab/tuple/apply.h>
#include <di/vocab/tuple/tie.h>
#include <di/vocab/tuple/tuple.h>
#include <di/vocab/tuple/tuple_cat.h>
#include <di/vocab/tuple/tuple_for_each.h>
#include <di/vocab/variant/holds_alternative.h>
#include <di/vocab/variant/variant.h>
#include <di/vocab/variant/visit.h>

namespace di::execution {
namespace when_all_ns {
    template<typename T>
    using DecayedRValue = meta::Decay<T>&&;

    template<typename Env, concepts::SenderIn<Env> Send>
    constexpr inline usize set_value_count = meta::Size<meta::ValueTypesOf<Send, Env, meta::List, meta::List>>;

    template<typename Env, typename... Sends>
    concept ValidSenders = (concepts::SenderIn<Sends, Env> && ...) && ((set_value_count<Env, Sends> < 2) && ...);

    template<typename Env, typename... Sends>
    constexpr inline bool never_sends_value = ((set_value_count<Env, Sends> == 0) || ...);

    template<typename Env, typename... Sends>
    using NonValueCompletions = meta::Unique<
        meta::PushBack<meta::Transform<meta::Concat<meta::ErrorTypesOf<Sends, Env, meta::List>...>,
                                       meta::Compose<meta::BindFront<meta::Quote<meta::AsLanguageFunction>, SetError>,
                                                     meta::Quote<meta::List>, meta::Quote<DecayedRValue>>>,
                       SetStopped()>>;

    template<typename Env, typename... Sends>
    requires(ValidSenders<Env, Sends...> && (!never_sends_value<Env, Sends...>) )
    using ValueCompletion = meta::AsLanguageFunction<
        SetValue, meta::Transform<meta::Concat<meta::ValueTypesOf<Sends, Env, meta::List, meta::TypeIdentity>...>,
                                  meta::Quote<DecayedRValue>>>;

    template<typename E>
    using Env = MakeEnv<E, With<Tag<get_stop_token>, sync::InPlaceStopToken>>;

    template<typename E, typename... Sends>
    struct CompletionSignaturesT
        : meta::TypeConstant<
              meta::AsTemplate<types::CompletionSignatures, meta::PushFront<NonValueCompletions<Env<E>, Sends...>,
                                                                            ValueCompletion<Env<E>, Sends...>>>> {};

    template<typename E, typename... Sends>
    requires(never_sends_value<E, Sends...>)
    struct CompletionSignaturesT<E, Sends...>
        : meta::TypeConstant<meta::AsTemplate<types::CompletionSignatures, NonValueCompletions<Env<E>, Sends...>>> {};

    template<typename E, typename... Sends>
    using Sigs = meta::Type<CompletionSignaturesT<E, Sends...>>;

    template<typename... Types>
    using DecayedOptionalTuple = vocab::Optional<vocab::Tuple<meta::Decay<Types>...>>;

    template<typename Env, typename... Sends>
    struct ValueStorageT : meta::TypeConstant<Void> {};

    template<typename Env, typename... Sends>
    requires(ValidSenders<Env, Sends...> && (!never_sends_value<Env, Sends...>) )
    struct ValueStorageT<Env, Sends...>
        : meta::TypeConstant<
              vocab::Tuple<meta::ValueTypesOf<Sends, Env, DecayedOptionalTuple, meta::TypeIdentity>...>> {};

    template<typename Env, typename... Sends>
    using ValueStorage = meta::Type<ValueStorageT<Env, Sends...>>;

    template<typename... Types>
    using DecayedVariant = meta::AsTemplate<vocab::Variant, meta::Unique<meta::List<meta::Decay<Types>...>>>;

    struct NotError {};
    struct Stopped {};

    template<typename Env, typename... Sends>
    using ErrorStorage =
        meta::AsTemplate<DecayedVariant,
                         meta::Concat<meta::List<NotError, Stopped>, meta::ErrorTypesOf<Sends, Env, meta::List>...>>;

    struct StopCallbackFunction {
        sync::InPlaceStopSource& stop_source;

        void operator()() const noexcept { stop_source.request_stop(); }
    };

    template<typename Rec, typename... Sends>
    struct DataT {
        struct Type {
        public:
            using Env = meta::EnvOf<Rec>;
            using Value = ValueStorage<Env, Sends...>;
            using Error = ErrorStorage<Env, Sends...>;
            using Count = usize;
            using StopToken = meta::StopTokenOf<Env>;
            using StopCallback = StopToken::template CallbackType<StopCallbackFunction>;

            explicit Type(Rec out_r_) : out_r(util::move(out_r_)) {}

            template<usize index, typename... Types>
            void report_value(Constexpr<index>, Types&&... values) {
                if constexpr (never_sends_value<Env, Sends...>) {
                    ((void) values, ...);
                } else {
                    auto& optional = util::get<index>(value);
                    optional.emplace(util::forward<Types>(values)...);
                }
                finish_one();
            }

            template<typename E>
            void report_error(E&& error) {
                auto old = failed.exchange(true, sync::MemoryOrder::AcquireRelease);
                if (!old) {
                    stop_source.request_stop();
                    this->error.template emplace<meta::Decay<E>>(util::forward<E>(error));
                }
                finish_one();
            }

            void report_stop() {
                auto old = failed.exchange(true, sync::MemoryOrder::AcquireRelease);
                if (!old) {
                    stop_source.request_stop();
                    error.template emplace<Stopped>();
                }
                finish_one();
            }

            void finish_one() {
                auto old_value = remaining.fetch_sub(1, sync::MemoryOrder::AcquireRelease);
                if (old_value == 1) {
                    // Reset the stop callback.
                    stop_callback.reset();

                    // Check if an error occurred. This does not need to be atomic because we're done and just used
                    // acquire release ordering.
                    if (!vocab::holds_alternative<NotError>(error)) {
                        if (!vocab::holds_alternative<Stopped>(error)) {
                            vocab::visit(
                                [&]<typename E>(E&& e) {
                                    if constexpr (!concepts::OneOf<meta::RemoveCVRef<E>, NotError, Stopped>) {
                                        execution::set_error(util::move(out_r), util::forward<E>(e));
                                    }
                                },
                                util::move(error));
                        } else {
                            execution::set_stopped(util::move(out_r));
                        }
                    } else {
                        if constexpr (!never_sends_value<Env, Sends...>) {
                            // Value is a Tuple<Optional<Vs...>, ...>, and we need to return all Vs's. Do this be
                            // concatenating all the tuples and then unpacking them.
                            auto lvalues = vocab::apply(
                                [](auto&... optionals) {
                                    return vocab::tuple_cat(vocab::apply(vocab::tie, *optionals)...);
                                },
                                value);
                            vocab::apply(
                                [&](auto&... values) {
                                    execution::set_value(util::move(out_r), util::move(values)...);
                                },
                                lvalues);
                        } else {
                            util::unreachable();
                        }
                    }
                }
            }

            [[no_unique_address]] Value value;
            [[no_unique_address]] Error error;
            [[no_unique_address]] Rec out_r;
            sync::InPlaceStopSource stop_source;
            sync::Atomic<Count> remaining { sizeof...(Sends) };
            sync::Atomic<bool> failed { false };
            vocab::Optional<StopCallback> stop_callback;
        };
    };

    template<concepts::Receiver Rec, concepts::Sender... Sends>
    using Data = meta::Type<DataT<Rec, Sends...>>;

    template<usize index, typename Send, typename Data>
    struct ReceiverT {
        struct Type {
            using is_receiver = void;

            Data* data;

            template<typename... Types>
            friend void tag_invoke(types::Tag<execution::set_value>, Type&& self, Types&&... values) {
                self.data->report_value(c_<index>, util::forward<Types>(values)...);
            }

            template<typename E>
            friend void tag_invoke(types::Tag<execution::set_error>, Type&& self, E&& error) {
                self.data->report_error(util::forward<E>(error));
            }

            friend void tag_invoke(types::Tag<execution::set_stopped>, Type&& self) { self.data->report_stop(); }

            friend auto tag_invoke(types::Tag<execution::get_env>, Type const& self) {
                return make_env(get_env(self.data->out_r),
                                with(get_stop_token, self.data->stop_source.get_stop_token()));
            }
        };
    };

    template<usize index, concepts::Sender Send, typename Data>
    using Receiver = meta::Type<ReceiverT<index, Send, Data>>;

    template<typename Rec, typename Indices, typename Sends>
    struct OperationStateT;

    template<typename Rec, usize... indices, typename... Sends>
    struct OperationStateT<Rec, meta::ListV<indices...>, meta::List<Sends...>> {
        struct Type : util::Immovable {
        public:
            using Data = when_all_ns::Data<Rec, Sends...>;
            using OpStates = vocab::Tuple<meta::ConnectResult<Sends, Receiver<indices, Sends, Data>>...>;

            explicit Type(Rec out_r, Sends&&... senders)
                : m_data(util::move(out_r))
                , m_op_states(util::DeferConstruct([&] -> meta::ConnectResult<Sends, Receiver<indices, Sends, Data>> {
                    return execution::connect(util::forward<Sends>(senders),
                                              Receiver<indices, Sends, Data> { util::addressof(m_data) });
                })...) {}

        private:
            friend void tag_invoke(types::Tag<execution::start>, Type& self) {
                // Emplace construct stop callback.
                self.m_data.stop_callback.emplace(execution::get_stop_token(execution::get_env(self.m_data.out_r)),
                                                  StopCallbackFunction { self.m_data.stop_source });

                // Check if stop requested:
                if (self.m_data.stop_source.stop_requested()) {
                    execution::set_stopped(util::move(self.m_data.out_r));
                    return;
                }

                // Call start on all operations.
                vocab::tuple_for_each(execution::start, self.m_op_states);
            }

            [[no_unique_address]] Data m_data;
            [[no_unique_address]] OpStates m_op_states;
        };
    };

    template<concepts::Receiver Rec, typename Indices, concepts::TypeList Sends>
    using OperationState = meta::Type<OperationStateT<Rec, Indices, Sends>>;

    template<typename... Senders>
    struct SenderT {
        struct Type {
        public:
            using is_sender = void;

            template<typename... Ts>
            explicit Type(InPlace, Ts&&... ts) : m_senders(util::forward<Ts>(ts)...) {}

        private:
            template<concepts::RemoveCVRefSameAs<Type> Self, typename E>
            requires(ValidSenders<Env<E>, meta::Like<Self, Senders>...>)
            friend auto tag_invoke(types::Tag<execution::get_completion_signatures>, Self&&, E&&)
                -> Sigs<E, meta::Like<Self, Senders>...>;

            template<concepts::RemoveCVRefSameAs<Type> Self, concepts::Receiver Rec>
            requires(concepts::ReceiverOf<Rec, meta::CompletionSignaturesOf<meta::Like<Self, Type>, meta::EnvOf<Rec>>>)
            friend auto tag_invoke(types::Tag<execution::connect>, Self&& self, Rec out_r) {
                return vocab::apply(
                    [&](auto&&... senders) {
                        return OperationState<Rec, meta::MakeIndexSequence<sizeof...(Senders)>,
                                              meta::List<meta::Like<Self, Senders>...>> {
                            util::move(out_r), util::forward<decltype(senders)>(senders)...
                        };
                    },
                    util::forward<Self>(self).m_senders);
            }

            vocab::Tuple<Senders...> m_senders;
        };
    };

    template<concepts::Sender... Senders>
    using Sender = meta::Type<SenderT<Senders...>>;

    struct Function {
        template<concepts::Sender... Senders>
        requires(sizeof...(Senders) > 0)
        concepts::Sender auto operator()(Senders&&... senders) const {
            if constexpr (concepts::TagInvocable<Function, Senders...>) {
                return function::tag_invoke(*this, util::forward<Senders>(senders)...);
            } else {
                return Sender<meta::RemoveCVRef<Senders>...> { in_place, util::forward<Senders>(senders)... };
            }
        }
    };

    struct VariantFunction {
        template<concepts::Sender... Senders>
        requires(sizeof...(Senders) > 0)
        concepts::Sender auto operator()(Senders&&... senders) const {
            if constexpr (concepts::TagInvocable<VariantFunction, Senders...>) {
                return function::tag_invoke(*this, util::forward<Senders>(senders)...);
            } else {
                return Function {}(execution::into_variant(util::forward<Senders>(senders))...);
            }
        }
    };

    struct TransferFunction {
        template<concepts::Scheduler Sched, concepts::Sender... Senders>
        requires(sizeof...(Senders) > 0)
        concepts::Sender auto operator()(Sched&& sched, Senders&&... senders) const {
            if constexpr (concepts::TagInvocable<TransferFunction, Sched, Senders...>) {
                return function::tag_invoke(*this, util::forward<Sched>(sched), util::forward<Senders>(senders)...);
            } else {
                return execution::transfer(Function {}(util::forward<Sender>(senders)...), util::forward<Sched>(sched));
            }
        }
    };

    struct TransferVariantFunction {
        template<concepts::Scheduler Sched, concepts::Sender... Senders>
        requires(sizeof...(Senders) > 0)
        concepts::Sender auto operator()(Sched&& sched, Senders&&... senders) const {
            if constexpr (concepts::TagInvocable<TransferVariantFunction, Sched, Senders...>) {
                return function::tag_invoke(*this, util::forward<Sched>(sched), util::forward<Senders>(senders)...);
            } else {
                return TransferFunction {}(util::forward<Sched>(sched),
                                           execution::into_variant(util::forward<Senders>(senders))...);
            }
        }
    };
}

constexpr inline auto when_all = when_all_ns::Function {};
constexpr inline auto when_all_with_variant = when_all_ns::VariantFunction {};
constexpr inline auto transfer_when_all = when_all_ns::TransferFunction {};
constexpr inline auto transfer_when_all_with_variant = when_all_ns::TransferVariantFunction {};
}

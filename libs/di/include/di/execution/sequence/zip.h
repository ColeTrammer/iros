#pragma once

#include <di/execution/algorithm/just.h>
#include <di/execution/algorithm/when_all.h>
#include <di/execution/concepts/sender.h>
#include <di/execution/query/is_always_lockstep_sequence.h>
#include <di/execution/query/make_env.h>
#include <di/execution/receiver/set_value.h>
#include <di/execution/sequence/into_lockstep_sequence.h>
#include <di/execution/sequence/into_variant_each.h>
#include <di/execution/sequence/sequence_sender.h>
#include <di/function/tag_invoke.h>
#include <di/meta/list/type.h>
#include <di/meta/remove_cvref.h>
#include <di/sync/stop_token/in_place_stop_source.h>
#include <di/sync/stop_token/in_place_stop_token.h>
#include <di/vocab/tuple/tuple.h>

namespace di::execution {
namespace zip_ns {
    using when_all_ns::Env;
    using when_all_ns::ErrorStorage;
    using when_all_ns::never_sends_value;
    using when_all_ns::NonValueCompletions;
    using when_all_ns::NotError;
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
                if (old_value == 1) {}
            }

            void report_outer_value() { finish_one_outer(); }

            template<typename E>
            void report_outer_error(E&& error) {
                auto old = outer_failed.exchange(true, sync::MemoryOrder::AcquireRelease);
                if (!old) {
                    stop_source.request_stop();
                    this->outer_error.template emplace<meta::Decay<E>>(util::forward<E>(error));
                }
                finish_one_outer();
            }

            void report_outer_stop() {
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

            [[no_unique_address]] Value inner_value;
            [[no_unique_address]] Error inner_error;
            sync::Atomic<Count> inner_remaining { sizeof...(Seqs) + 1 };
            sync::Atomic<bool> inner_failed { false };

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

    template<usize index, typename Send, typename Data>
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

            friend auto tag_invoke(Tag<set_next>, Type&, concepts::Sender auto&&) { return just(); }

            friend auto tag_invoke(Tag<get_env>, Type const& self) {
                return make_env(get_env(self.data->outer_out_r),
                                with(get_stop_token, self.data->stop_source.get_stop_token()));
            }
        };
    };

    template<usize index, concepts::Sender Send, typename Data>
    using Receiver = meta::Type<ReceiverT<index, Send, Data>>;

    template<typename Rec, typename Indices, typename Seqs>
    struct OperationStateT;

    template<typename Rec, usize... indices, typename... Seqs>
    struct OperationStateT<Rec, meta::ListV<indices...>, meta::List<Seqs...>> {
        struct Type : util::Immovable {
        public:
            using Data = OuterData<Rec, Seqs...>;
            using OpStates = vocab::Tuple<meta::SubscribeResult<Seqs, Receiver<indices, Seqs, Data>>...>;

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

                // Call start on all operations.
                vocab::tuple_for_each(start, self.m_op_states);
            }

            [[no_unique_address]] Data m_data;
            [[no_unique_address]] OpStates m_op_states;
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
            requires(ValidSenders<Env<E>, meta::Like<Self, Seqs>...> &&
                     (!never_sends_value<Env<E>, meta::Like<Self, Seqs>...>) )
            friend auto tag_invoke(Tag<get_completion_signatures>, Self&&, E&&)
                -> meta::AsTemplate<CompletionSignatures,
                                    meta::PushFront<NonValueCompletions<Env<E>, meta::Like<Self, Seqs>...>,
                                                    ValueCompletion<Env<E>, meta::Like<Self, Seqs>...>>>;

            template<concepts::RemoveCVRefSameAs<Type> Self, typename E>
            requires(ValidSenders<Env<E>, meta::Like<Self, Seqs>...> &&
                     (never_sends_value<Env<E>, meta::Like<Self, Seqs>...>) )
            friend auto tag_invoke(Tag<get_completion_signatures>, Self&&, E&&)
                -> meta::AsTemplate<CompletionSignatures, NonValueCompletions<Env<E>, meta::Like<Self, Seqs>...>>;

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
                return make_env(empty_env, with(is_always_lockstep_sequence, c_<true>));
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
/// ```cpp
/// namespace execution = di::execution;
///
/// auto sequence = zip(
///     from_container(di::Array { 1, 2, 3 }),
///     from_container(di::Array { 4, 5, 6 })
/// );
///
/// auto sum = 0;
/// ASSERT(sync_wait(ignore_all(sequence | then_each([&](int x, int y) {
///     sum += x * y;
/// }))));
/// ASSERT_EQ(sum, 32);
/// ```
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

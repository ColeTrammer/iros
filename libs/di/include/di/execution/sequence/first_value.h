#pragma once

#include "di/execution/concepts/receiver_of.h"
#include "di/execution/concepts/sender.h"
#include "di/execution/concepts/sender_in.h"
#include "di/execution/interface/connect.h"
#include "di/execution/interface/get_env.h"
#include "di/execution/meta/completion_signatures_of.h"
#include "di/execution/meta/connect_result.h"
#include "di/execution/meta/decayed_tuple.h"
#include "di/execution/meta/env_of.h"
#include "di/execution/meta/make_completion_signatures.h"
#include "di/execution/meta/variant_or_empty.h"
#include "di/execution/query/get_completion_signatures.h"
#include "di/execution/query/is_always_lockstep_sequence.h"
#include "di/execution/query/make_env.h"
#include "di/execution/receiver/receiver_adaptor.h"
#include "di/execution/receiver/set_stopped.h"
#include "di/execution/receiver/set_value.h"
#include "di/execution/sequence/sequence_sender.h"
#include "di/execution/types/completion_signuatures.h"
#include "di/function/invoke.h"
#include "di/function/pipeable.h"
#include "di/function/tag_invoke.h"
#include "di/meta/algorithm.h"
#include "di/meta/core.h"
#include "di/meta/language.h"
#include "di/meta/util.h"
#include "di/platform/compiler.h"
#include "di/sync/atomic.h"
#include "di/sync/memory_order.h"
#include "di/util/addressof.h"
#include "di/util/immovable.h"
#include "di/vocab/tuple/apply.h"
#include "di/vocab/tuple/tuple.h"
#include "di/vocab/variant/variant.h"
#include "di/vocab/variant/visit.h"

namespace di::execution {
namespace first_value_ns {
    template<typename Seq, typename Rec>
    struct DataT {
        struct Type {
            constexpr static bool is_lockstep = concepts::AlwaysLockstepSequence<Seq>;

            using Env = meta::EnvOf<Rec>;
            using Sigs = meta::PushFront<meta::AsList<meta::CompletionSignaturesOf<Seq, Env>>, SetStopped()>;
            using Tags =
                meta::Transform<Sigs,
                                meta::Compose<meta::Quote<meta::List>, meta::Quote<meta::LanguageFunctionReturn>>>;
            using DecayedArgs = meta::Transform<Sigs, meta::Quote<meta::AsList>>;
            using Storage = meta::AsTemplate<
                vocab::Variant,
                meta::Unique<meta::Transform<
                    meta::Zip<Tags, DecayedArgs>,
                    meta::Compose<meta::Uncurry<meta::Quote<meta::DecayedTuple>>, meta::Quote<meta::Join>>>>>;

            template<typename... Args>
            void complete_inner_value(Args&&... args) {
                complete_inner([&] {
                    storage.template emplace<meta::DecayedTuple<SetValue, Args...>>(set_value,
                                                                                    util::forward<Args>(args)...);
                });
            }

            template<typename E>
            void complete_inner_error(E&& error) {
                complete_inner([&] {
                    storage.template emplace<meta::DecayedTuple<SetError, E>>(set_error, util::forward<E>(error));
                });
            }

            void complete_inner_stopped() {
                complete_inner([&] {
                    storage.template emplace<meta::DecayedTuple<SetStopped>>(set_stopped);
                });
            }

            void complete_inner(concepts::Invocable<> auto&& emplace) {
                auto was_first = [&] {
                    if constexpr (is_lockstep) {
                        auto result = got_first_value.load(sync::MemoryOrder::Relaxed);
                        got_first_value.store(true, sync::MemoryOrder::Relaxed);
                        return !result;
                    } else {
                        return !got_first_value.exchange(true, sync::MemoryOrder::AcquireRelease);
                    }
                }();

                if (was_first) {
                    emplace();
                }
            }

            void complete_outer_value() {
                vocab::visit(
                    [&](auto&& tuple) {
                        vocab::apply(
                            [&](auto tag, auto&&... values) {
                                if constexpr (concepts::SameAs<SetValue, decltype(tag)>) {
                                    set_value(util::move(receiver), util::forward<decltype(values)>(values)...);
                                } else if constexpr (concepts::SameAs<SetError, decltype(tag)>) {
                                    set_error(util::move(receiver), util::forward<decltype(values)>(values)...);
                                } else if constexpr (concepts::SameAs<SetStopped, decltype(tag)>) {
                                    set_stopped(util::move(receiver));
                                }
                            },
                            util::forward<decltype(tuple)>(tuple));
                    },
                    util::move(storage));
            }

            template<typename E>
            void complete_outer_error(E&& error) {
                auto decayed_error = util::forward<E>(error);
                set_error(util::move(receiver), util::move(decayed_error));
            }

            void complete_outer_stopped() { set_stopped(util::move(receiver)); }

            [[no_unique_address]] Rec receiver;
            [[no_unique_address]] Storage storage {};
            sync::Atomic<bool> got_first_value { false };
        };
    };

    template<typename Seq, typename Rec>
    using Data = meta::Type<DataT<Seq, Rec>>;

    template<typename Seq, typename Rec, typename Next, typename R>
    struct NextDataT {
        struct Type {
            [[no_unique_address]] R next_receiver;
        };
    };

    template<typename Seq, typename Rec, typename Next, typename R>
    using NextData = meta::Type<NextDataT<Seq, Rec, Next, R>>;

    template<typename Seq, typename Rec, typename Next, typename R>
    struct NextReceiverT {
        struct Type : ReceiverAdaptor<Type> {
        private:
            using Base = ReceiverAdaptor<Type>;
            friend Base;

        public:
            explicit Type(NextData<Seq, Rec, Next, R>* next_data, Data<Seq, Rec>* data)
                : m_next_data(next_data), m_data(data) {}

            auto base() const& -> R const& { return m_next_data->next_receiver; }
            auto base() && -> R&& { return util::move(m_next_data->next_receiver); }

        private:
            template<typename... Args>
            void set_value(Args&&... values) && {
                m_data->complete_inner_value(util::forward<Args>(values)...);
                execution::set_stopped(util::move(*this).base());
            }

            template<typename E>
            void set_error(E&& error) && {
                m_data->complete_inner_error(util::forward<E>(error));
                execution::set_stopped(util::move(*this).base());
            }

            void set_stopped() && {
                m_data->complete_inner_stopped();
                execution::set_stopped(util::move(*this).base());
            }

            NextData<Seq, Rec, Next, R>* m_next_data;
            Data<Seq, Rec>* m_data;
        };
    };

    template<typename Seq, typename Rec, typename Next, typename R>
    using NextReceiver = meta::Type<NextReceiverT<Seq, Rec, Next, R>>;

    template<typename Seq, typename Rec, typename Next, typename R>
    struct NextOperationStateT {
        struct Type : util::Immovable {
        public:
            using Op = meta::ConnectResult<Next, NextReceiver<Seq, Rec, Next, R>>;

            explicit Type(Data<Seq, Rec>* data, Next&& next_sender, R receiver)
                : m_next_data(util::move(receiver))
                , m_data(data)
                , m_operation(connect(util::forward<Next>(next_sender),
                                      NextReceiver<Seq, Rec, Next, R>(util::addressof(m_next_data), data))) {}

        private:
            friend void tag_invoke(types::Tag<start>, Type& self) { start(self.m_operation); }

            NextData<Seq, Rec, Next, R> m_next_data;
            Data<Seq, Rec>* m_data;
            DI_IMMOVABLE_NO_UNIQUE_ADDRESS Op m_operation;
        };
    };

    template<typename Seq, typename Rec, typename Next, typename R>
    using NextOperationState = meta::Type<NextOperationStateT<Seq, Rec, Next, R>>;

    template<typename Seq, typename Rec, typename Next>
    struct NextSenderT {
        struct Type {
            using is_sender = void;

            using CompletionSignatures = types::CompletionSignatures<SetStopped()>;

            [[no_unique_address]] Next next;
            Data<Seq, Rec>* data;

            template<concepts::ReceiverOf<CompletionSignatures> R>
            friend auto tag_invoke(Tag<connect>, Type&& self, R receiver) {
                return NextOperationState<Seq, Rec, Next, R> { self.data, util::move(self.next), util::move(receiver) };
            }

            friend auto tag_invoke(types::Tag<get_env>, Type const& self) { return make_env(get_env(self.next)); }
        };
    };

    template<typename Seq, typename Rec, typename Next>
    using NextSender = meta::Type<NextSenderT<Seq, Rec, meta::RemoveCVRef<Next>>>;

    template<typename Seq, typename Rec>
    struct ReceiverT {
        struct Type {
            using is_receiver = void;

            Data<Seq, Rec>* data;

            friend void tag_invoke(Tag<set_value>, Type&& self) { self.data->complete_outer_value(); }

            template<typename E>
            friend void tag_invoke(Tag<set_error>, Type&& self, E&& error) {
                self.data->complete_outer_error(util::forward<E>(error));
            }

            friend void tag_invoke(Tag<set_stopped>, Type&& self) { self.data->complete_outer_stopped(); }

            template<concepts::Sender Next>
            friend auto tag_invoke(Tag<set_next>, Type& self, Next&& next) {
                return NextSender<Seq, Rec, Next> { util::forward<Next>(next), self.data };
            }

            friend auto tag_invoke(Tag<get_env>, Type const& self) { return make_env(get_env(self.data->receiver)); }
        };
    };

    template<typename Seq, typename Rec>
    using Receiver = meta::Type<ReceiverT<Seq, Rec>>;

    template<typename Seq, typename Rec>
    struct OperationStateT {
        struct Type : util::Immovable {
        public:
            using Op = meta::SubscribeResult<Seq, Receiver<Seq, Rec>>;

            explicit Type(Seq&& sequence, Rec receiver)
                : m_data(util::move(receiver))
                , m_operation(subscribe(util::forward<Seq>(sequence), Receiver<Seq, Rec>(util::addressof(m_data)))) {}

        private:
            friend void tag_invoke(types::Tag<start>, Type& self) { start(self.m_operation); }

            Data<Seq, Rec> m_data;
            DI_IMMOVABLE_NO_UNIQUE_ADDRESS Op m_operation;
        };
    };

    template<typename Seq, typename Rec>
    using OperationState = meta::Type<OperationStateT<Seq, Rec>>;

    template<typename... Values>
    using SigSetValue = CompletionSignatures<SetValue(meta::Decay<Values>&&...)>;

    template<typename E>
    using SigSetError = CompletionSignatures<SetError(meta::Decay<E>&&)>;

    template<typename Seq, typename Env>
    using Sigs =
        meta::MakeCompletionSignatures<Seq, MakeEnv<Env>, CompletionSignatures<SetStopped()>, SigSetValue, SigSetError>;

    template<typename Seq>
    struct SenderT {
        struct Type {
            using is_sender = void;

            [[no_unique_address]] Seq sequence;

            template<concepts::RemoveCVRefSameAs<Type> Self, typename Env>
            friend auto tag_invoke(Tag<get_completion_signatures>, Self&&, Env&&) -> Sigs<meta::Like<Self, Seq>, Env> {
                return {};
            }

            template<concepts::RemoveCVRefSameAs<Type> Self, typename Rec>
            requires(concepts::ReceiverOf<Rec, Sigs<meta::Like<Self, Seq>, meta::EnvOf<Rec>>>)
            friend auto tag_invoke(Tag<connect>, Self&& self, Rec receiver) {
                return OperationState<meta::Like<Self, Seq>, Rec>(util::forward<Self>(self).sequence,
                                                                  util::move(receiver));
            }

            friend auto tag_invoke(Tag<get_env>, Type const& self) { return make_env(get_env(self.sequence)); }
        };
    };

    template<typename Seq>
    using Sender = meta::Type<SenderT<meta::RemoveCVRef<Seq>>>;

    struct Function : function::pipeline::EnablePipeline {
        template<concepts::Sender Seq>
        auto operator()(Seq&& sequence) const {
            if constexpr (concepts::TagInvocable<Function, Seq>) {
                static_assert(concepts::Sender<meta::TagInvokeResult<Function, Seq>>,
                              "first_value() customizations must return a Sender");
                return tag_invoke(*this, util::forward<Seq>(sequence));
            } else {
                return Sender<Seq>(util::forward<Seq>(sequence));
            }
        }
    };
}

/// @brief Transform a sequence into a sender of its first value.
///
/// @param sequence The sequence to transform.
///
/// @return A sender of the first value of the sequence.
///
/// This function will return the first error or value of the sequence after it completes. Any errors or values after
/// the first will be ignored. However, if the cleanup action of the sequence sends an error, that error will be sent in
/// place of the first value. If the sequence itself is empty, the returned sender will complete with a stopped signal.
///
/// See the following for an example:
/// @snippet{trimleft} tests/test_execution_sequence.cpp first_value
constexpr inline auto first_value = first_value_ns::Function {};
}

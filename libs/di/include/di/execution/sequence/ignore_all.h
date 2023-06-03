#pragma once

#include <di/concepts/remove_cvref_same_as.h>
#include <di/concepts/same_as.h>
#include <di/execution/concepts/receiver.h>
#include <di/execution/concepts/receiver_of.h>
#include <di/execution/concepts/sender.h>
#include <di/execution/concepts/sender_to.h>
#include <di/execution/interface/connect.h>
#include <di/execution/interface/start.h>
#include <di/execution/meta/connect_result.h>
#include <di/execution/meta/env_of.h>
#include <di/execution/meta/error_types_of.h>
#include <di/execution/meta/make_completion_signatures.h>
#include <di/execution/meta/sends_stopped.h>
#include <di/execution/query/get_completion_signatures.h>
#include <di/execution/receiver/receiver_adaptor.h>
#include <di/execution/receiver/set_error.h>
#include <di/execution/receiver/set_stopped.h>
#include <di/execution/receiver/set_value.h>
#include <di/execution/sequence/sequence_sender.h>
#include <di/execution/types/completion_signuatures.h>
#include <di/function/pipeable.h>
#include <di/function/tag_invoke.h>
#include <di/meta/conditional.h>
#include <di/meta/decay.h>
#include <di/meta/like.h>
#include <di/meta/list/id.h>
#include <di/meta/list/list.h>
#include <di/meta/list/type.h>
#include <di/meta/remove_cvref.h>
#include <di/platform/compiler.h>
#include <di/sync/atomic.h>
#include <di/sync/memory_order.h>
#include <di/util/addressof.h>
#include <di/util/immovable.h>
#include <di/util/unreachable.h>
#include <di/vocab/variant/visit.h>

namespace di::execution {
namespace ignore_all_ns {
    template<typename T>
    using DecayedRValue = meta::Decay<T>&&;

    template<typename Env, typename Seq>
    constexpr bool sends_stopped =
        meta::sends_stopped<Seq, Env> || (meta::Size<meta::ErrorTypesOf<Seq, Env, meta::List>> > 0);

    template<typename... Types>
    using DecayedVariant = vocab::Variant<meta::Decay<Types>...>;

    struct NotError {};
    struct Stopped {};

    template<typename Env, typename Seq>
    using ErrorStorage = meta::AsTemplate<
        DecayedVariant,
        meta::Unique<meta::Concat<
            meta::Conditional<sends_stopped<Env, Seq>, meta::List<NotError, Stopped>, meta::List<NotError>>,
            meta::ErrorTypesOf<Seq, Env, meta::List>>>>;

    template<typename Seq, typename Rec>
    struct DataT {
        struct Type {
            using Env = meta::EnvOf<Rec>;
            using Error = ErrorStorage<Env, Seq>;

            explicit Type(Rec receiver_) : receiver(util::move(receiver_)) {}

            template<typename E>
            void report_error(E&& error) {
                auto old = failed.exchange(true, sync::MemoryOrder::AcquireRelease);
                if (!old) {
                    this->error.template emplace<meta::Decay<E>>(util::forward<E>(error));
                }
            }

            void report_stop() {
                auto old = failed.exchange(true, sync::MemoryOrder::AcquireRelease);
                if (!old) {
                    this->error.template emplace<Stopped>();
                }
            }

            void finish() {
                auto did_fail = failed.load(sync::MemoryOrder::Acquire);
                if (!did_fail) {
                    return set_value(util::move(receiver));
                }

                vocab::visit(
                    [&]<typename E>(E&& error) {
                        if constexpr (concepts::SameAs<meta::Decay<E>, Stopped>) {
                            return set_stopped(util::move(receiver));
                        } else if constexpr (concepts::SameAs<meta::Decay<E>, NotError>) {
                            util::unreachable();
                        } else {
                            return set_error(util::move(receiver), util::forward<E>(error));
                        }
                    },
                    util::move(error));
            }

            [[no_unique_address]] Rec receiver;
            Error error;
            sync::Atomic<bool> failed { false };
        };
    };

    template<concepts::Sender Seq, concepts::Receiver Rec>
    using Data = meta::Type<DataT<Seq, Rec>>;

    template<typename Seq, typename Rec, typename Next, typename NextRec>
    struct NextDataT {
        struct Type {
            [[no_unique_address]] NextRec next_receiver;
        };
    };

    template<concepts::Sender Seq, concepts::Receiver Rec, concepts::Sender Next, concepts::Receiver NextRec>
    using NextData = meta::Type<NextDataT<Seq, Rec, Next, NextRec>>;

    template<typename Seq, typename Rec, typename Next, typename NextRec>
    struct NextReceiverT {
        struct Type : ReceiverAdaptor<Type> {
        private:
            using Base = ReceiverAdaptor<Type>;
            friend Base;

        public:
            explicit Type(NextData<Seq, Rec, Next, NextRec>* next_data, Data<Seq, Rec>* data)
                : m_next_data(next_data), m_data(data) {}

            NextRec const& base() const& { return m_next_data->next_receiver; }
            NextRec&& base() && { return util::move(m_next_data->next_receiver); }

        private:
            void set_value(auto&&...) && { execution::set_value(util::move(*this).base()); }

            template<typename E>
            void set_error(E&& error) && {
                m_data->report_error(util::forward<E>(error));
                execution::set_stopped(util::move(*this).base());
            }

            void set_stopped() && {
                m_data->report_stop();
                execution::set_stopped(util::move(*this).base());
            }

            NextData<Seq, Rec, Next, NextRec>* m_next_data;
            Data<Seq, Rec>* m_data;
        };
    };

    template<concepts::Sender Seq, concepts::Receiver Rec, concepts::Sender Next, concepts::Receiver NextRec>
    using NextReceiver = meta::Type<NextReceiverT<Seq, Rec, Next, NextRec>>;

    template<typename Seq, typename Rec, typename Next, typename NextRec>
    struct NextOperationStateT {
        struct Type : util::Immovable {
            using Op = meta::ConnectResult<Next, NextReceiver<Seq, Rec, Next, NextRec>>;

            explicit Type(Data<Seq, Rec>* data, Next&& next_sender, NextRec next_receiver)
                : m_next_data(util::move(next_receiver))
                , m_operation(connect(util::forward<Next>(next_sender),
                                      NextReceiver<Seq, Rec, Next, NextRec>(util::addressof(m_next_data), data))) {}

        private:
            friend void tag_invoke(types::Tag<start>, Type& self) { start(self.m_operation); }

            [[no_unique_address]] NextData<Seq, Rec, Next, NextRec> m_next_data;
            DI_IMMOVABLE_NO_UNIQUE_ADDRESS Op m_operation;
        };
    };

    template<concepts::Sender Seq, concepts::Receiver Rec, concepts::Sender Next, concepts::Receiver NextRec>
    using NextOperationState = meta::Type<NextOperationStateT<Seq, Rec, Next, NextRec>>;

    template<typename Next, typename Env>
    using NextSignatures =
        meta::MakeCompletionSignatures<Next, Env, types::CompletionSignatures<SetValue()>,
                                       meta::Id<types::CompletionSignatures<>>::template Invoke,
                                       meta::Id<types::CompletionSignatures<SetStopped()>>::template Invoke>;

    template<typename Seq, typename Rec, typename Next>
    struct NextSenderT {
        struct Type {
            using is_sender = void;

            [[no_unique_address]] Next next;
            Data<Seq, Rec>* data;

            template<concepts::RemoveCVRefSameAs<Type> Self, typename NextRec>
            requires(concepts::ReceiverOf<NextRec, NextSignatures<meta::Like<Self, Next>, meta::EnvOf<NextRec>>>)
            friend auto tag_invoke(types::Tag<connect>, Self&& self, NextRec receiver) {
                return NextOperationState<Seq, Rec, meta::Like<Self, Next>, NextRec>(
                    self.data, util::forward<Self>(self).next, util::move(receiver));
            }

            template<concepts::RemoveCVRefSameAs<Type> Self, typename Env>
            friend auto tag_invoke(types::Tag<get_completion_signatures>, Self&&, Env&&)
                -> NextSignatures<meta::Like<Self, Next>, Env>;

            constexpr friend decltype(auto) tag_invoke(types::Tag<get_env>, Type const& self) {
                return get_env(self.next);
            }
        };
    };

    template<concepts::Sender Seq, concepts::Receiver Rec, concepts::Sender Next>
    using NextSender = meta::Type<NextSenderT<Seq, Rec, Next>>;

    template<typename Seq, typename Rec>
    struct ReceiverT {
        struct Type : ReceiverAdaptor<Type> {
        private:
            using Base = ReceiverAdaptor<Type>;
            friend Base;

        public:
            explicit Type(Data<Seq, Rec>* data) : m_data(data) {}

            Rec const& base() const& { return m_data->receiver; }
            Rec&& base() && { return util::move(m_data->receiver); }

        private:
            void set_value() && { m_data->finish(); }

            template<concepts::Sender Next>
            auto set_next(Next&& next) & {
                return NextSender<Seq, Rec, meta::RemoveCVRef<Next>>(util::forward<Next>(next), m_data);
            }

            Data<Seq, Rec>* m_data;
        };
    };

    template<concepts::Sender Seq, concepts::Receiver Rec>
    using Receiver = meta::Type<ReceiverT<Seq, Rec>>;

    template<typename Seq, typename Rec>
    struct OperationStateT {
        struct Type : util::Immovable {
        public:
            using Op = meta::SubscribeResult<Seq, Receiver<Seq, Rec>>;

            explicit Type(Seq&& sender, Rec receiver)
                : m_data(util::move(receiver))
                , m_operation(subscribe(util::forward<Seq>(sender), Receiver<Seq, Rec>(util::addressof(m_data)))) {}

        private:
            friend void tag_invoke(types::Tag<start>, Type& self) { start(self.m_operation); }

            [[no_unique_address]] Data<Seq, Rec> m_data;
            DI_IMMOVABLE_NO_UNIQUE_ADDRESS Op m_operation;
        };
    };

    template<concepts::Sender Seq, concepts::Receiver Rec>
    using OperationState = meta::Type<OperationStateT<Seq, Rec>>;

    template<typename Err>
    using ErrorsWithStopped = types::CompletionSignatures<SetStopped(), SetError(DecayedRValue<Err>)>;

    template<typename Seq, typename Env>
    using Completions =
        meta::MakeCompletionSignatures<Seq, Env, types::CompletionSignatures<SetValue()>,
                                       meta::Id<types::CompletionSignatures<>>::template Invoke, ErrorsWithStopped>;

    template<typename Seq>
    struct SenderT {
        struct Type {
            using is_sender = void;

            [[no_unique_address]] Seq sequence;

            template<concepts::RemoveCVRefSameAs<Type> Self, typename Rec>
            requires(concepts::ReceiverOf<Rec, Completions<meta::Like<Self, Seq>, meta::EnvOf<Rec>>>)
            friend auto tag_invoke(types::Tag<connect>, Self&& self, Rec receiver) {
                return OperationState<meta::Like<Self, Seq>, Rec>(util::forward<Self>(self).sequence,
                                                                  util::move(receiver));
            }

            template<concepts::RemoveCVRefSameAs<Type> Self, typename Env>
            friend auto tag_invoke(types::Tag<get_completion_signatures>, Self&&, Env&&)
                -> Completions<meta::Like<Self, Seq>, Env>;

            constexpr friend decltype(auto) tag_invoke(types::Tag<get_env>, Type const& self) {
                return get_env(self.sequence);
            }
        };
    };

    template<concepts::Sender Seq>
    using Sender = meta::Type<SenderT<meta::RemoveCVRef<Seq>>>;

    struct Function : function::pipeline::EnablePipeline {
        template<concepts::Sender Seq>
        concepts::Sender auto operator()(Seq&& sequence) const {
            if constexpr (concepts::TagInvocable<Function, Seq>) {
                return function::tag_invoke(*this, util::forward<Seq>(sequence));
            } else {
                return Sender<Seq> { util::forward<Seq>(sequence) };
            }
        }
    };
}

/// @brief Adapt a sequence sender to a regular sender of void, ignoring the sequence's values.
///
/// @param sequence The sequence sender to adapt.
///
/// @return A sender which sends an empty completion when the sequence completes.
///
/// If the sequence sender sends an error, the first error encountered is decay copied and forwarded to the receiver
/// once the sequence completes. If instead the first non-value completion sent is a stop, the receiver is sent a stop
/// when the sequence completes. Otherwise, the sent values are discarded and the receiver is sent an empty completion
/// when the sequence completes.
///
/// @warning Calling di::execution::ignore_all() with a sequence sender corresponding to an infinte sequence will result
/// in a sender which never completes.
constexpr inline auto ignore_all = ignore_all_ns::Function {};
}

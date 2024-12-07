#pragma once

#include "di/execution/concepts/operation_state.h"
#include "di/execution/concepts/prelude.h"
#include "di/execution/concepts/receiver.h"
#include "di/execution/concepts/receiver_of.h"
#include "di/execution/concepts/sender.h"
#include "di/execution/concepts/sender_in.h"
#include "di/execution/concepts/sender_to.h"
#include "di/execution/concepts/valid_completion_signatures.h"
#include "di/execution/interface/connect.h"
#include "di/execution/interface/get_env.h"
#include "di/execution/interface/start.h"
#include "di/execution/meta/completion_signatures_of.h"
#include "di/execution/meta/env_of.h"
#include "di/execution/meta/make_completion_signatures.h"
#include "di/execution/meta/stop_token_of.h"
#include "di/execution/query/is_debug_env.h"
#include "di/execution/query/make_env.h"
#include "di/execution/receiver/set_stopped.h"
#include "di/execution/receiver/set_value.h"
#include "di/execution/sequence/async_range.h"
#include "di/execution/types/completion_signuatures.h"
#include "di/execution/types/empty_env.h"
#include "di/function/invoke.h"
#include "di/function/tag_invoke.h"
#include "di/meta/algorithm.h"
#include "di/meta/core.h"
#include "di/sync/concepts/stoppable_token.h"
#include "di/sync/concepts/unstoppable_token.h"
#include "di/util/declval.h"
#include "di/util/immovable.h"
#include "di/util/move.h"

/// @file sequence_sender.h
/// @brief Defines the sequence sender concepts and related CPOs.
///
/// This mechanism is modelled after a draft c++ [standard
/// proposal](https://github.com/kirkshoop/sequence-next), which builds on [P2300](https://wg21.link/p2300).

namespace di::concepts {
/// @brief A sender that can be returned from di::execution::set_next().
///
/// @tparam Sender The sender type.
/// @tparam Env The environment type.
///
/// Senders returned from di::execution::set_next() must have a completion signature which only contains
/// di::execution::SetValue() and di::execution::SetStopped().
template<typename Sender, typename Env = types::EmptyEnv>
concept NextSender =
    SenderIn<Sender, Env> &&
    (meta::Size<meta::Unique<meta::Concat<meta::List<execution::SetValue(), execution::SetStopped()>,
                                          meta::AsList<meta::CompletionSignaturesOf<Sender, Env>>>>> == 2);
}

namespace di::execution {
namespace set_next_ns {
    struct Function {
        template<concepts::Receiver Rec, concepts::Sender Send>
        requires(concepts::TagInvocable<Function, Rec&, Send>)
        auto operator()(Rec& receiver, Send&& sender) const {
            static_assert(concepts::NextSender<meta::TagInvokeResult<Function, Rec&, Send>>,
                          "The return value of execution::set_next() must model di::concepts::NextSender.");
            return function::tag_invoke(*this, receiver, util::forward<Send>(sender));
        }
    };
}

/// @brief Set the next sender of a sequence
///
/// @param receiver The receiver of the sequence.
/// @param sender The sender to set as the next sender in the sequence.
///
/// @returns A sender which will adapt the values sent by @p sender to model di::concepts::NextSender.
///
/// See the @subpage md_docs_2di_2execution document for more information on the async sequence model.
constexpr inline auto set_next = set_next_ns::Function {};
}

namespace di::meta {
template<typename Rec, typename Send>
using NextSenderOf = decltype(execution::set_next(util::declval<meta::RemoveCVRef<Rec>&>(), util::declval<Send>()));
}

namespace di::execution {
struct SequenceTag {};

template<typename S>
constexpr inline bool enable_sequence_sender = false;

template<typename S>
requires(requires { typename S::is_sender; } && concepts::SameAs<SequenceTag, typename S::is_sender>)
constexpr inline bool enable_sequence_sender<S> = true;

template<typename S>
requires(concepts::AwaitableAsyncRange<S>)
constexpr inline bool enable_sequence_sender<S> = true;
}

namespace di::concepts {
template<typename Send>
concept SequenceSender = Sender<Send> && execution::enable_sequence_sender<meta::RemoveCVRef<Send>>;

template<typename Send, typename Env = types::EmptyEnv>
concept SequenceSenderIn = SenderIn<Send, Env> && SequenceSender<Send>;
}

namespace di::execution::dummy_ns {
struct DummyOperationState : di::Immovable {
    friend void tag_invoke(types::Tag<execution::start>, DummyOperationState&) {}
};

template<concepts::ValidCompletionSignatures Sigs>
struct DummySenderOf {
    using is_sender = void;

    using CompletionSignatures = Sigs;

    friend auto tag_invoke(types::Tag<execution::connect>, DummySenderOf, auto&&) -> DummyOperationState { return {}; }
};

template<concepts::ValidCompletionSignatures Sigs>
constexpr inline auto dummy_sender_of = DummySenderOf<Sigs> {};
}

namespace di::concepts {
template<typename Rec, typename Sigs>
concept SubscriberOf =
    ReceiverOf<Rec, types::CompletionSignatures<execution::SetValue()>> && requires(meta::RemoveCVRef<Rec>& receiver) {
        execution::set_next(receiver, execution::dummy_ns::dummy_sender_of<Sigs>);
    };
}

namespace di::execution::sender_to_sequence_adaptor_ns {
template<typename Rec>
struct ReceiverT {
    struct Type {
        using is_receiver = void;

        using Env = meta::EnvOf<Rec>;
        using StopToken = meta::StopTokenOf<Env>;

        [[no_unique_address]] Rec receiver;

        friend void tag_invoke(types::Tag<set_value>, Type&& self)
        requires(concepts::Invocable<SetValue, Rec>)
        {
            execution::set_value(util::move(self.receiver));
        }

        friend void tag_invoke(types::Tag<set_stopped>, Type&& self)
        requires(concepts::Invocable<SetValue, Rec> &&
                 (concepts::UnstoppableToken<StopToken> || concepts::Invocable<SetStopped, Rec>) )
        {
            if constexpr (concepts::UnstoppableToken<StopToken>) {
                execution::set_value(util::move(self.receiver));
            } else {
                concepts::StoppableToken auto token = get_stop_token(get_env(self.receiver));
                if (token.stop_requested()) {
                    execution::set_stopped(util::move(self.receiver));
                } else {
                    execution::set_value(util::move(self.receiver));
                }
            }
        }

        friend auto tag_invoke(types::Tag<get_env>, Type const& self) { return make_env(get_env(self.receiver)); }
    };
};

template<concepts::Receiver Rec>
using Receiver = meta::Type<ReceiverT<meta::RemoveCVRef<Rec>>>;
}

namespace di::meta {
template<typename Send, typename Env>
using SequenceCompletionSignaturesOf =
    meta::MakeCompletionSignatures<Send, Env, types::CompletionSignatures<execution::SetValue()>,
                                   meta::TypeConstant<types::CompletionSignatures<>>::template Invoke>;
}

namespace di::concepts {
namespace detail {
    template<typename Rec, typename Send>
    concept AdaptableToSequence =
        Receiver<Rec> && SenderIn<Send, meta::EnvOf<Rec>> &&
        SubscriberOf<Rec, meta::CompletionSignaturesOf<Send, meta::EnvOf<Rec>>> &&
        !concepts::SequenceSenderIn<Send, meta::EnvOf<Rec>> &&
        concepts::SenderTo<meta::NextSenderOf<Rec, Send>, execution::sender_to_sequence_adaptor_ns::Receiver<Rec>>;
}

template<typename Rec, typename Send>
concept SubscriberFrom = Receiver<Rec> && SenderIn<Send, meta::EnvOf<Rec>> &&
                         SubscriberOf<Rec, meta::CompletionSignaturesOf<Send, meta::EnvOf<Rec>>> &&
                         ((concepts::SequenceSenderIn<Send, meta::EnvOf<Rec>> &&
                           concepts::ReceiverOf<Rec, meta::SequenceCompletionSignaturesOf<Send, meta::EnvOf<Rec>>>) ||
                          detail::AdaptableToSequence<Rec, Send>);
}

namespace di::execution {
namespace subscribe_ns {
    struct Function {
        template<concepts::Receiver Rec, concepts::SenderIn<meta::EnvOf<Rec>> Seq>
        requires(concepts::detail::AdaptableToSequence<Rec, Seq> ||
                 (concepts::SubscriberFrom<Rec, Seq> && concepts::TagInvocable<Function, Seq, Rec>) ||
                 (concepts::DebugEnv<meta::EnvOf<Rec>>) )
        auto operator()(Seq&& sequence, Rec&& receiver) const {
            if constexpr (concepts::detail::AdaptableToSequence<Rec, Seq>) {
                return connect(
                    set_next(receiver, util::forward<Seq>(sequence)),
                    execution::sender_to_sequence_adaptor_ns::Receiver<Rec> { util::forward<Rec>(receiver) });
            } else if constexpr (concepts::SubscriberFrom<Rec, Seq> && concepts::TagInvocable<Function, Seq, Rec>) {
                static_assert(
                    concepts::OperationState<meta::TagInvokeResult<Function, Seq, Rec>>,
                    "The return value of di::execution::subscribe() must model di::concepts::OperationState.");
                return function::tag_invoke(*this, util::forward<Seq>(sequence), util::forward<Rec>(receiver));
            } else if constexpr (!concepts::SequenceSender<Seq>) {
                return connect(
                    set_next(receiver, util::forward<Seq>(sequence)),
                    execution::sender_to_sequence_adaptor_ns::Receiver<Rec> { util::forward<Rec>(receiver) });
            } else {
                return function::tag_invoke(*this, util::forward<Seq>(sequence), util::forward<Rec>(receiver));
            }
        }
    };
}

/// @brief Subscribe a receiver to a sequence.
///
/// @param sequence The sequence to subscribe to.
/// @param receiver The receiver to subscribe.
///
/// @returns An operation state which models di::concepts::OperationState.
constexpr inline auto subscribe = subscribe_ns::Function {};
}

namespace di::meta {
template<typename Send, typename Rec>
using SubscribeResult = decltype(execution::subscribe(util::declval<Send>(), util::declval<Rec>()));
}

namespace di::concepts {
template<typename Send, typename Rec>
concept SequenceSenderTo =
    SequenceSenderIn<Send, Rec> && SubscriberFrom<Rec, Send> && requires(Send&& sender, Rec&& receiver) {
        execution::subscribe(util::forward<Send>(sender), util::forward<Rec>(receiver));
    };
}

namespace di {
using execution::SequenceTag;
}

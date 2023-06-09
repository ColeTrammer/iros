#pragma once

#include <di/concepts/copy_constructible.h>
#include <di/concepts/remove_cvref_same_as.h>
#include <di/execution/concepts/receiver.h>
#include <di/execution/concepts/sender.h>
#include <di/execution/concepts/sender_to.h>
#include <di/execution/interface/connect.h>
#include <di/execution/interface/get_env.h>
#include <di/execution/meta/completion_signatures_of.h>
#include <di/execution/query/get_completion_signatures.h>
#include <di/execution/query/make_env.h>
#include <di/execution/receiver/receiver_adaptor.h>
#include <di/function/curry.h>
#include <di/function/tag_invoke.h>
#include <di/meta/constexpr.h>
#include <di/meta/decay.h>
#include <di/meta/like.h>
#include <di/meta/remove_cvref.h>

namespace di::execution {
namespace with_env_ns {
    template<typename Rec, typename Env>
    struct ReceiverT {
        struct Type : ReceiverAdaptor<Type, Rec> {
        private:
            using Base = ReceiverAdaptor<Type, Rec>;
            friend Base;

        public:
            explicit Type(Rec reciever, Env env) : Base(util::move(reciever)), m_env(util::move(env)) {}

        private:
            Env get_env() const& { return m_env; }

            Env m_env;
        };
    };

    template<concepts::Receiver Rec, typename Env>
    using Receiver = meta::Type<ReceiverT<Rec, Env>>;

    template<typename Send, typename Env>
    struct SenderT {
        struct Type {
            using is_sender = void;

            [[no_unique_address]] Send m_sender;
            [[no_unique_address]] Env m_env;

        private:
            template<concepts::RemoveCVRefSameAs<Type> Self, typename En>
            friend auto tag_invoke(types::Tag<get_completion_signatures>, Self&&, En&&)
                -> meta::CompletionSignaturesOf<meta::Like<Self, Send>, Env>;

            template<concepts::RemoveCVRefSameAs<Type> Self, concepts::Receiver Rec>
            requires(concepts::SenderTo<meta::Like<Self, Send>, Receiver<Rec, Env>>)
            friend auto tag_invoke(types::Tag<connect>, Self&& self, Rec receiver) {
                return connect(util::forward<Self>(self).m_sender,
                               Receiver<Rec, Env> { util::move(receiver), util::forward<Self>(self).m_env });
            }

            decltype(auto) tag_invoke(types::Tag<get_env>, Type const& self) { return get_env(self.m_sender); }
        };
    };

    template<concepts::Sender Send, typename Env>
    using Sender = meta::Type<SenderT<meta::RemoveCVRef<Send>, meta::Decay<Env>>>;

    struct Function {
        template<concepts::CopyConstructible Env, concepts::Sender Send>
        auto operator()(Env&& env, Send&& sender) const {
            return Sender<Send, Env> { util::forward<Send>(sender), util::forward<Env>(env) };
        }
    };
}

/// @brief Adapts a sender to run with a specified environment.
///
/// @param env The environment to run the sender with.
/// @param sender The sender to adapt.
///
/// @returns A sender that runs the specified sender with the specified environment.
///
/// This functions allows injecting an environment into a sender. The environment is accessible within the sender using
/// execution::read(), and allows the sender to dynamically get things like a stop token, allocator, or current
/// scheduler.
///
/// The following is an example of a sender that uses its injected stop token to know if it should stop itself:
///
/// @snippet{trimleft} tests/test_execution.cpp with_env
///
/// @see with
/// @see make_env
/// @see read
constexpr inline auto with_env = function::curry(with_env_ns::Function {}, c_<2zu>);
}

#pragma once

#include <di/execution/concepts/receiver.h>
#include <di/execution/concepts/sender.h>
#include <di/execution/concepts/sender_to.h>
#include <di/execution/interface/connect.h>
#include <di/execution/interface/get_env.h>
#include <di/execution/meta/completion_signatures_of.h>
#include <di/execution/query/get_completion_signatures.h>
#include <di/execution/query/is_debug_env.h>
#include <di/execution/query/make_env.h>
#include <di/execution/receiver/receiver_adaptor.h>
#include <di/execution/types/empty_env.h>
#include <di/function/curry.h>
#include <di/function/pipeable.h>
#include <di/function/tag_invoke.h>
#include <di/meta/constexpr.h>
#include <di/meta/core.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>

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
            auto get_env() const& -> Env { return m_env; }

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
                -> meta::CompletionSignaturesOf<meta::Like<Self, Send>, Env> {
                return {};
            }

            template<concepts::RemoveCVRefSameAs<Type> Self, concepts::Receiver Rec>
            requires(concepts::SenderTo<meta::Like<Self, Send>, Receiver<Rec, Env>>)
            friend auto tag_invoke(types::Tag<connect>, Self&& self, Rec receiver) {
                return connect(util::forward_like<Self>(self.m_sender),
                               Receiver<Rec, Env> { util::move(receiver), util::forward_like<Self>(self.m_env) });
            }

            auto tag_invoke(types::Tag<get_env>, Type const& self) -> decltype(auto) { return get_env(self.m_sender); }
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

    struct DebugFunction : function::pipeline::EnablePipeline {
        template<concepts::Sender Send>
        auto operator()(Send&& sender) const {
            using Env = MakeEnv<EmptyEnv, With<Tag<is_debug_env>, Constexpr<true>>>;
            return Sender<Send, Env> { util::forward<Send>(sender), make_env(empty_env, with(is_debug_env, c_<true>)) };
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

/// @brief Adapts a sender to run with a debug environment.
///
/// @param sender The sender to adapt.
///
/// @returns A sender that runs the specified sender with a debug environment.
///
/// This function is equivalent to calling with_env() with a debug environment.
///
/// @see with_env
/// @see is_debug_env
constexpr inline auto with_debug_env = with_env_ns::DebugFunction {};
}

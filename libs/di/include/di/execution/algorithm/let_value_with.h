#pragma once

#include <di/execution/concepts/prelude.h>
#include <di/execution/meta/prelude.h>
#include <di/execution/receiver/prelude.h>
#include <di/execution/types/prelude.h>
#include <di/function/invoke.h>
#include <di/meta/util.h>
#include <di/platform/compiler.h>
#include <di/util/defer_construct.h>
#include <di/util/move.h>
#include <di/vocab/tuple/apply.h>
#include <di/vocab/tuple/tuple.h>

namespace di::execution {
namespace let_value_with_ns {
    template<typename State, typename Send, typename Rec>
    struct OperationStateT {
        struct Type : util::Immovable {
            template<typename Fun, typename Factories>
            explicit Type(Fun&& function, Factories&& factories, Rec receiver)
                : m_state(util::DeferConstruct([&] {
                    return vocab::apply(
                        [&]<typename... Fs>(Fs&&... fs) {
                            return State(util::DeferConstruct([&] {
                                return function::invoke(util::forward<Fs>(fs));
                            })...);
                        },
                        util::forward<Factories>(factories));
                }))
                , m_op_state(execution::connect(
                      [&] {
                          return vocab::apply(
                              [&](auto&... args) {
                                  return function::invoke(util::forward<Fun>(function), args...);
                              },
                              m_state);
                      }(),
                      util::move(receiver))) {}

        private:
            friend void tag_invoke(types::Tag<execution::start>, Type& self) { execution::start(self.m_op_state); }

            DI_IMMOVABLE_NO_UNIQUE_ADDRESS State m_state;
            DI_IMMOVABLE_NO_UNIQUE_ADDRESS meta::ConnectResult<Send, Rec> m_op_state;
        };
    };

    template<typename State, concepts::Sender Send, concepts::Receiver Rec>
    using OperationState = meta::Type<OperationStateT<State, Send, Rec>>;

    struct Function {
        template<typename Fun, typename... Factories>
        struct SenderT {
            struct Type {
            private:
                using State = vocab::Tuple<meta::Decay<meta::InvokeResult<Factories&&>>...>;
                using Send = meta::InvokeResult<Fun, meta::Decay<meta::InvokeResult<Factories&&>>&...>;

            public:
                using is_sender = void;

                [[no_unique_address]] Fun function;
                [[no_unique_address]] vocab::Tuple<Factories...> factories;

                template<typename F, typename... Fa>
                explicit Type(F&& f, Fa&&... fa) : function(util::forward<F>(f)), factories(util::forward<Fa>(fa)...) {}

            private:
                template<concepts::DecaysTo<Type> Self, typename Rec>
                requires(concepts::DecayConstructible<meta::Like<Self, Fun>> &&
                         (concepts::DecayConstructible<meta::Like<Self, Factories>> && ...) &&
                         concepts::SenderTo<Send, Rec>)
                friend auto tag_invoke(types::Tag<connect>, Self&& self, Rec receiver) {
                    auto function = util::forward<Self>(self).function;
                    auto factories = util::forward<Self>(self).factories;
                    return OperationState<State, Send, Rec> { util::move(function), util::move(factories),
                                                              util::move(receiver) };
                }

                template<concepts::DecaysTo<Type> Self, typename Env>
                friend auto tag_invoke(types::Tag<get_completion_signatures>, Self&&, Env&&)
                    -> meta::CompletionSignaturesOf<Send, Env> {
                    return {};
                }
            };
        };

        template<typename Fun, typename... Factories>
        using Sender = meta::Type<SenderT<Fun, Factories...>>;

        template<concepts::MovableValue... Factories, concepts::MovableValue Fun, typename DFun = meta::Decay<Fun>>
        requires(concepts::Sender<
                 meta::InvokeResult<DFun &&, meta::Decay<meta::InvokeResult<meta::Decay<Factories> &&>>&...>>)
        concepts::Sender auto operator()(Fun&& function, Factories&&... factories) const {
            return Sender<Fun, Factories...> { util::forward<Fun>(function), util::forward<Factories>(factories)... };
        }
    };
}

/// @brief Inject values into an operation state.
///
/// @param function The function to invoke with the injected values.
/// @param factories The factories to use to create the injected values.
///
/// @returns A sender that injects the values into the operation state and invokes the function.
///
/// This function allows the injection of values into an operation state. These are given the the provided function as
/// lvalues. And the function returns a sender which is allowed access to these values. This is particularly useful for
/// creating immovable objects, which cannot be stored in a sender because senders must be movable.
///
/// The following example creates an injects a regular value, as well as an immovable value into the operation state.
/// This demonstrates the function::make_deferred helper function, which binds arguments to a constructor, as well as
/// the fact that normal lambdas can be used as a factory function.
///
/// @snippet{trimleft} tests/test_execution.cpp let_value_with
///
/// @see function::make_deferred
constexpr inline auto let_value_with = let_value_with_ns::Function {};
}

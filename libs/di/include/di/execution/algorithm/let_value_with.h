#pragma once

#include <di/concepts/movable_value.h>
#include <di/execution/concepts/prelude.h>
#include <di/execution/meta/prelude.h>
#include <di/execution/receiver/prelude.h>
#include <di/execution/types/prelude.h>

namespace di::execution {
namespace let_value_with_ns {
    template<typename State, typename Send, typename Rec>
    struct OperationStateT {
        struct Type {
            template<typename Factory, typename Fun>
            explicit Type(Factory& factory, Fun& function, Rec receiver)
                : m_state(function::invoke(factory))
                , m_op_state(execution::connect(function::invoke(function, m_state), util::move(receiver))) {}

        private:
            friend void tag_invoke(types::Tag<execution::start>, Type& self) { execution::start(self.m_op_state); }

            State m_state;
            meta::ConnectResult<Send, Rec> m_op_state;
        };
    };

    template<typename State, concepts::Sender Send, concepts::Receiver Rec>
    using OperationState = meta::Type<OperationStateT<State, Send, Rec>>;

    struct Function {
        template<typename Factory, typename Fun>
        struct SenderT {
            struct Type {
            private:
                using State = meta::InvokeResult<Factory>;
                using Send = meta::InvokeResult<Fun, State&>;

            public:
                [[no_unique_address]] Factory factory;
                [[no_unique_address]] Fun function;

            private:
                template<concepts::DecaysTo<Type> Self, typename Rec>
                requires(concepts::SenderTo<Send, Rec>)
                friend auto tag_invoke(types::Tag<connect>, Self&& self, Rec receiver) {
                    return OperationState<State, Send, Rec> { self.factory, self.function, util::move(receiver) };
                }

                template<concepts::DecaysTo<Type> Self, typename Env>
                friend auto tag_invoke(types::Tag<get_completion_signatures>, Self&&, Env)
                    -> types::DependentCompletionSignatures<Env>;

                template<concepts::DecaysTo<Type> Self, typename Env>
                friend auto tag_invoke(types::Tag<get_completion_signatures>, Self&&, Env)
                    -> meta::CompletionSignaturesOf<Send, Env>
                requires(true);
            };
        };

        template<concepts::MovableValue Factory, concepts::MovableValue Fun>
        using Sender = meta::Type<SenderT<Factory, Fun>>;

        template<concepts::MovableValue Factory, concepts::MovableValue Fun, typename DF = meta::Decay<Factory>,
                 typename DFun = meta::Decay<Fun>>
        requires(concepts::Invocable<DF&> && concepts::Invocable<DFun&, meta::InvokeResult<DF>&> &&
                 concepts::Sender<meta::InvokeResult<DFun&, meta::InvokeResult<DF>&>>)
        concepts::Sender auto operator()(Factory&& factory, Fun&& function) const {
            return Sender<DF, DFun> { util::forward<Factory>(factory), util::forward<Fun>(function) };
        }
    };
}

constexpr inline auto let_value_with = let_value_with_ns::Function {};
}

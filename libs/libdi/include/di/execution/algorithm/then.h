#pragma once

#include <di/concepts/decays_to.h>
#include <di/concepts/movable_value.h>
#include <di/execution/concepts/forwarding_receiver_query.h>
#include <di/execution/concepts/forwarding_sender_query.h>
#include <di/execution/concepts/operation_state.h>
#include <di/execution/concepts/receiver_of.h>
#include <di/execution/concepts/sender.h>
#include <di/execution/meta/connect_result.h>
#include <di/execution/meta/make_completion_signatures.h>
#include <di/execution/query/get_completion_scheduler.h>
#include <di/function/curry_back.h>

namespace di::execution {
namespace then_ns {
    template<typename Rec, typename Fun>
    struct ReceiverT {
        struct Type {
        public:
            [[no_unique_address]] Rec receiver;
            [[no_unique_address]] Fun function;

        private:
            template<typename... Args>
            requires(concepts::Invocable<Fun, Args...> && concepts::LanguageVoid<meta::InvokeResult<Fun, Args...>> &&
                     concepts::ReceiverOf<Rec, types::CompletionSignatures<SetValue()>>)
            friend void tag_invoke(SetValue, Type&& self, Args&&... args) {
                function::invoke(util::move(self.function), util::forward<Args>(args)...);
                set_value(util::move(self.receiver));
            }

            template<typename... Args>
            friend void tag_invoke(SetValue, Type&& self, Args&&... args)
            requires(concepts::Invocable<Fun, Args...> &&
                     concepts::ReceiverOf<Rec, types::CompletionSignatures<SetValue(meta::InvokeResult<Fun, Args...>)>>)
            {
                set_value(util::move(self.receiver), function::invoke(util::move(self.function), util::forward<Args>(args)...));
            }

            template<concepts::OneOf<SetError, SetStopped> Tag, typename... Args>
            friend void tag_invoke(Tag tag, Type&& self, Args&&... args)
            requires(requires { tag(util::move(self.receiver), util::forward<Args>(args)...); })
            {
                tag(util::move(self.receiver), util::forward<Args>(args)...);
            }

            template<concepts::ForwardingReceiverQuery Tag, typename... Args>
            constexpr friend auto tag_invoke(Tag tag, Type const& self, Args&&... args)
                -> decltype(tag(self.receiver, util::forward<Args>(args)...)) {
                return tag(self.receiver, util::forward<Args>(args)...);
            }
        };
    };

    template<typename Rec, typename Fun>
    using Receiver = meta::Type<ReceiverT<meta::Decay<Rec>, meta::Decay<Fun>>>;

    template<typename Tag, typename... Args>
    struct ComplSig : meta::TypeConstant<Tag(Args...)> {};

    template<typename Tag>
    struct ComplSig<Tag, void> : meta::TypeConstant<Tag()> {};

    template<typename Send, typename Fun>
    struct SenderT {
        struct Type {
        public:
            [[no_unique_address]] Send sender;
            [[no_unique_address]] Fun function;

        private:
            template<typename... Args>
            using SetValueCompletions = types::CompletionSignatures<meta::Type<ComplSig<SetValue, meta::InvokeResult<Fun, Args...>>>>;

            template<concepts::DecaysTo<Type> Self, typename Rec>
            requires(concepts::SenderTo<meta::Like<Self, Send>, Receiver<Rec, Fun>>)
            friend auto tag_invoke(types::Tag<connect>, Self&& self, Rec receiver) {
                return connect(util::forward<Self>(self).sender,
                               Receiver<Rec, Fun> { util::move(receiver), util::forward<Self>(self).function });
            }

            template<concepts::DecaysTo<Type> Self, typename Env>
            friend auto tag_invoke(types::Tag<get_completion_signatures>, Self&&, Env)
                -> meta::MakeCompletionSignatures<meta::Like<Self, Send>, Env, types::CompletionSignatures<>, SetValueCompletions>;

            template<concepts::ForwardingSenderQuery Tag, typename... Args>
            constexpr friend auto tag_invoke(Tag tag, Type const& self, Args&&... args)
                -> decltype(tag(self.sender, util::forward<Args>(args)...)) {
                return tag(self.sender, util::forward<Args>(args)...);
            }
        };
    };

    template<typename Send, typename Fun>
    using Sender = meta::Type<SenderT<meta::Decay<Send>, meta::Decay<Fun>>>;

    struct Function {
        template<concepts::Sender Send, concepts::MovableValue Fun>
        concepts::Sender auto operator()(Send&& sender, Fun&& function) const {
            if constexpr (requires {
                              function::tag_invoke(*this, get_completion_scheduler<SetValue>(sender), util::forward<Send>(sender),
                                                   util::forward<Fun>(function));
                          }) {
                return function::tag_invoke(*this, get_completion_scheduler<SetValue>(sender), util::forward<Send>(sender),
                                            util::forward<Fun>(function));
            } else if constexpr (requires { function::tag_invoke(*this, util::forward<Send>(sender), util::forward<Fun>(function)); }) {
                return function::tag_invoke(*this, util::forward<Send>(sender), util::forward<Fun>(function));
            } else {
                return Sender<Send, Fun> { util::forward<Send>(sender), util::forward<Fun>(function) };
            }
        }
    };
}

constexpr inline auto then = function::curry_back(then_ns::Function {}, meta::size_constant<2>);
}
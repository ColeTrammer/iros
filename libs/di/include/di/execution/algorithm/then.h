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
#include <di/execution/receiver/receiver_adaptor.h>
#include <di/function/curry_back.h>
#include <di/meta/unwrap_expected.h>

namespace di::execution {
namespace then_ns {
    template<concepts::Receiver Rec, typename Fun>
    struct ReceiverT {
        struct Type : private ReceiverAdaptor<Type, Rec> {
        private:
            using Base = ReceiverAdaptor<Type, Rec>;
            friend Base;

        public:
            explicit Type(Rec receiver, Fun function) : Base(util::move(receiver)), m_function(util::move(function)) {}

        private:
            template<typename... Args>
            requires(concepts::Invocable<Fun, Args...> &&
                     concepts::LanguageVoid<meta::UnwrapExpected<InvokeResult<Fun, Args...>>> &&
                     concepts::ReceiverOf<Rec, types::CompletionSignatures<SetValue()>>)
            void set_value(Args&&... args) && {
                if constexpr (concepts::Expected<meta::InvokeResult<Fun, Args...>>) {
                    auto result = function::invoke(util::move(m_function), util::forward<Args>(args)...);
                    if (!result) {
                        execution::set_error(util::move(*this).base(), util::move(result).error());
                    } else {
                        execution::set_value(util::move(*this).base());
                    }
                } else {
                    function::invoke(util::move(m_function), util::forward<Args>(args)...);
                    execution::set_value(util::move(*this).base());
                }
            }

            template<typename... Args>
            requires(concepts::Invocable<Fun, Args...> &&
                     concepts::ReceiverOf<
                         Rec, types::CompletionSignatures<SetValue(meta::UnwrapExpected<InvokeResult<Fun, Args...>>)>>)
            void set_value(Args&&... args) && {
                if constexpr (concepts::Expected<meta::InvokeResult<Fun, Args...>>) {
                    auto result = function::invoke(util::move(m_function), util::forward<Args>(args)...);
                    if (!result) {
                        execution::set_error(util::move(*this).base(), util::move(result).error());
                    } else {
                        execution::set_value(util::move(*this).base(), util::move(result).value());
                    }
                } else {
                    execution::set_value(util::move(*this).base(),
                                         function::invoke(util::move(m_function), util::forward<Args>(args)...));
                }
            }

            [[no_unique_address]] Fun m_function;
        };
    };

    template<typename Rec, typename Fun>
    using Receiver = meta::Type<ReceiverT<meta::Decay<Rec>, meta::Decay<Fun>>>;

    template<typename Tag, typename... Args>
    struct ComplSig : meta::TypeConstant<Tag(Args...)> {};

    template<typename Tag>
    struct ComplSig<Tag, void> : meta::TypeConstant<Tag()> {};

    template<typename R>
    struct ErrorComplSigs : meta::TypeConstant<meta::List<>> {};

    template<concepts::Expected R>
    requires(!concepts::LanguageVoid<meta::ExpectedError<R>>)
    struct ErrorComplSigs<R> : meta::TypeConstant<meta::List<SetError(meta::ExpectedError<R>)>> {};

    template<typename Send, typename Fun>
    struct SenderT {
        struct Type {
        public:
            [[no_unique_address]] Send sender;
            [[no_unique_address]] Fun function;

        private:
            template<typename... Args>
            using ValueCompletion =
                meta::List<meta::Type<ComplSig<SetValue, meta::UnwrapExpected<meta::InvokeResult<Fun, Args...>>>>>;

            template<typename... Args>
            using ValueErrorCompletion = meta::Type<ErrorComplSigs<meta::InvokeResult<Fun, Args...>>>;

            template<typename... Args>
            using SetValueCompletions =
                meta::AsTemplate<CompletionSignatures,
                                 meta::Concat<ValueCompletion<Args...>, ValueErrorCompletion<Args...>>>;

            template<concepts::DecaysTo<Type> Self, typename Rec>
            requires(concepts::DecayConstructible<meta::Like<Self, Send>> &&
                     concepts::SenderTo<meta::Like<Self, Send>, Receiver<Rec, Fun>>)
            friend auto tag_invoke(types::Tag<connect>, Self&& self, Rec receiver) {
                return connect(util::forward<Self>(self).sender,
                               Receiver<Rec, Fun> { util::move(receiver), util::forward<Self>(self).function });
            }

            template<concepts::DecaysTo<Type> Self, typename Env>
            friend auto tag_invoke(types::Tag<get_completion_signatures>, Self&&, Env)
                -> meta::MakeCompletionSignatures<meta::Like<Self, Send>, Env, types::CompletionSignatures<>,
                                                  SetValueCompletions>;

            template<concepts::ForwardingSenderQuery Tag, typename... Args>
            constexpr friend auto tag_invoke(Tag tag, Type const& self, Args&&... args)
                -> meta::InvokeResult<Tag, Send const&, Args...> {
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
                              function::tag_invoke(*this, get_completion_scheduler<SetValue>(sender),
                                                   util::forward<Send>(sender), util::forward<Fun>(function));
                          }) {
                return function::tag_invoke(*this, get_completion_scheduler<SetValue>(sender),
                                            util::forward<Send>(sender), util::forward<Fun>(function));
            } else if constexpr (requires {
                                     function::tag_invoke(*this, util::forward<Send>(sender),
                                                          util::forward<Fun>(function));
                                 }) {
                return function::tag_invoke(*this, util::forward<Send>(sender), util::forward<Fun>(function));
            } else {
                return Sender<Send, Fun> { util::forward<Send>(sender), util::forward<Fun>(function) };
            }
        }
    };
}

constexpr inline auto then = function::curry_back(then_ns::Function {}, meta::size_constant<2>);
}

#pragma once

#include <di/execution/concepts/prelude.h>
#include <di/execution/concepts/receiver.h>
#include <di/execution/interface/connect.h>
#include <di/execution/interface/get_env.h>
#include <di/execution/meta/env_of.h>
#include <di/execution/meta/prelude.h>
#include <di/execution/query/make_env.h>
#include <di/execution/receiver/prelude.h>
#include <di/execution/types/prelude.h>
#include <di/function/tag_invoke.h>
#include <di/meta/core.h>
#include <di/meta/language.h>
#include <di/meta/util.h>
#include <di/util/defer_construct.h>
#include <di/util/immovable.h>

namespace di::execution {
namespace let_ns {
    template<typename CPO, typename Completions>
    using ArgTypes = meta::Unique<
        meta::Transform<meta::Filter<meta::AsList<Completions>, meta::IsFunctionTo<CPO>>,
                        meta::Compose<meta::Uncurry<meta::Quote<meta::DecayedTuple>>, meta::Quote<meta::AsList>>>>;

    template<typename CPO, typename Fun, typename Completions>
    using SenderTypes = meta::Transform<
        ArgTypes<CPO, Completions>,
        meta::Compose<meta::Uncurry<meta::BindFront<meta::Quote<meta::InvokeResult>, Fun>>,
                      meta::BindBack<meta::Quote<meta::Transform>, meta::Quote<meta::AddLValueReference>>,
                      meta::Quote<meta::AsList>>>;

    template<typename CPO, typename Rec, typename Fun, typename Completions>
    struct DataT {
        struct Type {
        private:
            using ArgsStorage = meta::AsTemplate<Variant, meta::PushFront<ArgTypes<CPO, Completions>, Void>>;

            using ConnectResults = meta::Unique<meta::Transform<SenderTypes<CPO, Fun, Completions>,
                                                                meta::BindBack<meta::Quote<meta::ConnectResult>, Rec>>>;

            using OpStateStorage = meta::AsTemplate<Variant, meta::PushFront<ConnectResults, Void>>;

        public:
            explicit Type(Fun f_, Rec out_r_) : f(util::move(f_)), out_r(util::move(out_r_)) {}

            [[no_unique_address]] Fun f;
            [[no_unique_address]] Rec out_r;
            [[no_unique_address]] ArgsStorage args {};
            [[no_unique_address]] OpStateStorage op_state3 {};

            template<typename... Args>
            requires((concepts::DecayConstructible<Args> && ...) && concepts::Invocable<Fun, meta::Decay<Args>&...>)
            void phase2(Args&&... args) {
                using Tuple = meta::DecayedTuple<Args...>;
                auto& decayed_args = this->args.template emplace<Tuple>(util::forward<Args>(args)...);

                using Sender = meta::InvokeResult<Fun, meta::Decay<Args>&...>;
                using OpState = meta::ConnectResult<Sender, Rec>;

                auto& op_state = this->op_state3.template emplace<OpState>(util::DeferConstruct([&] {
                    return execution::connect(apply(util::move(f), decayed_args), util::move(out_r));
                }));

                execution::start(op_state);
            }
        };
    };

    template<concepts::OneOf<SetValue, SetError, SetStopped> CPO, concepts::Receiver Rec, concepts::MovableValue Fun,
             concepts::InstanceOf<CompletionSignatures> Completions>
    using Data = meta::Type<DataT<CPO, Rec, Fun, Completions>>;

    template<typename CPO, typename Rec, typename Fun, typename Completions>
    struct ReceiverT {
        struct Type {
            using is_receiver = void;

            Data<CPO, Rec, Fun, Completions>* data { nullptr };

        private:
            template<concepts::SameAs<CPO> Tag, typename... Args>
            friend void tag_invoke(Tag, Type&& self, Args&&... args)
            requires(requires {
                util::declval<Data<CPO, Rec, Fun, Completions>&>().phase2(util::forward<Args>(args)...);
            })
            {
                self.data->phase2(util::forward<Args>(args)...);
            }

            template<concepts::OneOf<SetValue, SetError, SetStopped> Tag, typename... Args>
            friend void tag_invoke(Tag tag, Type&& self, Args&&... args)
            requires(!concepts::SameAs<Tag, CPO> && concepts::Invocable<Tag, Rec, Args...>)
            {
                return tag(util::move(self.data->out_r), util::forward<Args>(args)...);
            }

            constexpr friend auto tag_invoke(types::Tag<get_env>, Type const& self) {
                return make_env(get_env(self.data->out_r));
            }
        };
    };

    template<concepts::OneOf<SetValue, SetError, SetStopped> CPO, concepts::Receiver Rec, concepts::MovableValue Fun,
             concepts::InstanceOf<CompletionSignatures> Completions>
    using Receiver = meta::Type<ReceiverT<CPO, Rec, Fun, Completions>>;

    template<typename CPO, typename Send, typename Rec, typename Fun>
    struct OperationStateT {
        struct Type : util::Immovable {
        private:
            using Completions = meta::CompletionSignaturesOf<Send, MakeEnv<meta::EnvOf<Rec>>>;

        public:
            explicit Type(Fun f, Rec out_r, Send&& sender)
                : m_data(util::move(f), util::move(out_r))
                , m_op_state2(execution::connect(util::forward<Send>(sender),
                                                 Receiver<CPO, Rec, Fun, Completions> { util::addressof(m_data) })) {}

        private:
            friend void tag_invoke(types::Tag<start>, Type& self) { execution::start(self.m_op_state2); }

            [[no_unique_address]] Data<CPO, Rec, Fun, Completions> m_data;
            DI_IMMOVABLE_NO_UNIQUE_ADDRESS meta::ConnectResult<Send, Receiver<CPO, Rec, Fun, Completions>> m_op_state2;
        };
    };

    template<concepts::OneOf<SetValue, SetError, SetStopped> CPO, concepts::Sender Send, concepts::Receiver Rec,
             concepts::MovableValue Fun>
    using OperationState = meta::Type<OperationStateT<CPO, Send, Rec, Fun>>;

    template<typename CPO, typename Send, typename Fun>
    struct SenderT {
        struct Type {
        public:
            using is_sender = void;

            [[no_unique_address]] Send sender;
            [[no_unique_address]] Fun function;

        private:
            template<concepts::DecaysTo<Type> Self, typename Env>
            using BaseCompletionSignatures =
                meta::Filter<meta::AsList<meta::CompletionSignaturesOf<meta::Like<Self, Send>, Env>>,
                             meta::Not<meta::IsFunctionTo<CPO>>>;

            template<concepts::DecaysTo<Type> Self, typename Env>
            using InvokeCompletionSignatures = meta::Join<
                meta::Transform<SenderTypes<CPO, Fun, meta::CompletionSignaturesOf<meta::Like<Self, Send>, Env>>,
                                meta::Compose<meta::Quote<meta::AsList>,
                                              meta::BindBack<meta::Quote<meta::CompletionSignaturesOf>, Env>>>>;

            template<concepts::DecaysTo<Type> Self, typename Env>
            using CompletionSignatures = meta::AsTemplate<
                CompletionSignatures,
                meta::Unique<meta::Concat<BaseCompletionSignatures<Self, Env>, InvokeCompletionSignatures<Self, Env>>>>;

            template<concepts::DecaysTo<Type> Self, typename Env>
            friend auto tag_invoke(types::Tag<get_completion_signatures>, Self&&, Env&&)
                -> CompletionSignatures<Self, MakeEnv<Env>>;

            template<concepts::DecaysTo<Type> Self, concepts::Receiver Rec,
                     concepts::SameAs<types::Tag<connect>> Tag = types::Tag<connect>>
            requires(concepts::DecayConstructible<meta::Like<Self, Fun>> &&
                     concepts::SenderTo<
                         meta::Like<Self, Send>,
                         Receiver<CPO, Rec, Fun,
                                  meta::CompletionSignaturesOf<meta::Like<Self, Send>, MakeEnv<meta::EnvOf<Rec>>>>>)
            friend auto tag_invoke(Tag, Self&& self, Rec receiver) {
                return OperationState<CPO, meta::Like<Self, Send>, Rec, Fun> { util::forward<Self>(self).function,
                                                                               util::move(receiver),
                                                                               util::forward<Self>(self).sender };
            }

            constexpr friend auto tag_invoke(types::Tag<get_env>, Type const& self) {
                return make_env(get_env(self.sender));
            }
        };
    };

    template<concepts::OneOf<SetValue, SetError, SetStopped> CPO, concepts::Sender Send, concepts::MovableValue Fun>
    using Sender = meta::Type<SenderT<CPO, meta::RemoveCVRef<Send>, meta::Decay<Fun>>>;

    template<concepts::OneOf<SetValue, SetError, SetStopped> CPO>
    struct Function {
        template<concepts::Sender Send, concepts::MovableValue Fun>
        concepts::Sender auto operator()(Send&& sender, Fun&& function) const {
            if constexpr (requires {
                              function::tag_invoke(*this, get_completion_scheduler<SetValue>(get_env(sender)),
                                                   util::forward<Send>(sender), util::forward<Fun>(function));
                          }) {
                return function::tag_invoke(*this, get_completion_scheduler<SetValue>(get_env(sender)),
                                            util::forward<Send>(sender), util::forward<Fun>(function));
            } else if constexpr (requires {
                                     function::tag_invoke(*this, util::forward<Send>(sender),
                                                          util::forward<Fun>(function));
                                 }) {
                return function::tag_invoke(*this, util::forward<Send>(sender), util::forward<Fun>(function));
            } else {
                return Sender<CPO, Send, Fun> { util::forward<Send>(sender), util::forward<Fun>(function) };
            }
        }
    };
}

constexpr inline auto let_value = function::curry_back(let_ns::Function<SetValue> {}, meta::c_<2zu>);
constexpr inline auto let_error = function::curry_back(let_ns::Function<SetError> {}, meta::c_<2zu>);
constexpr inline auto let_stopped = function::curry_back(let_ns::Function<SetStopped> {}, meta::c_<2zu>);
}

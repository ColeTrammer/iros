#pragma once

#include <di/concepts/decay_convertible.h>
#include <di/concepts/expected.h>
#include <di/concepts/language_void.h>
#include <di/concepts/movable_value.h>
#include <di/concepts/one_of.h>
#include <di/concepts/remove_cvref_same_as.h>
#include <di/concepts/same_as.h>
#include <di/execution/concepts/receiver_of.h>
#include <di/execution/concepts/sender.h>
#include <di/execution/interface/connect.h>
#include <di/execution/interface/get_env.h>
#include <di/execution/interface/start.h>
#include <di/execution/meta/completion_signatures_of.h>
#include <di/execution/meta/connect_result.h>
#include <di/execution/meta/decayed_tuple.h>
#include <di/execution/meta/env_of.h>
#include <di/execution/query/get_completion_signatures.h>
#include <di/execution/query/make_env.h>
#include <di/execution/receiver/set_error.h>
#include <di/execution/receiver/set_stopped.h>
#include <di/execution/receiver/set_value.h>
#include <di/execution/types/completion_signuatures.h>
#include <di/function/curry_back.h>
#include <di/function/invoke.h>
#include <di/function/tag_invoke.h>
#include <di/meta/decay.h>
#include <di/meta/expected_error.h>
#include <di/meta/expected_value.h>
#include <di/meta/like.h>
#include <di/meta/list/as_list.h>
#include <di/meta/list/as_template.h>
#include <di/meta/list/concat.h>
#include <di/meta/list/filter.h>
#include <di/meta/list/is_function_to.h>
#include <di/meta/list/join.h>
#include <di/meta/list/not.h>
#include <di/meta/list/push_back.h>
#include <di/meta/list/quote.h>
#include <di/meta/list/transform.h>
#include <di/meta/list/type.h>
#include <di/meta/list/unique.h>
#include <di/meta/remove_cvref.h>
#include <di/meta/type_constant.h>
#include <di/platform/compiler.h>
#include <di/util/addressof.h>

namespace di::execution {
namespace then_ns {
    template<typename CPO, typename Completions>
    using PassthroughSignatures = meta::Filter<meta::AsList<Completions>, meta::Not<meta::IsFunctionTo<CPO>>>;

    template<typename CPO, typename Completions>
    using ArgTypes =
        meta::Transform<meta::Filter<meta::AsList<Completions>, meta::IsFunctionTo<CPO>>, meta::Quote<meta::AsList>>;

    template<typename CPO, typename Fun, typename Completions>
    using ResultTypes = meta::Transform<ArgTypes<CPO, Completions>,
                                        meta::Uncurry<meta::BindFront<meta::Quote<meta::InvokeResult>, Fun>>>;

    template<typename T>
    struct ComplSigT : meta::TypeConstant<SetValue(T)> {};

    template<>
    struct ComplSigT<void> : meta::TypeConstant<SetValue()> {};

    template<typename T>
    using ComplSig = meta::Type<ComplSigT<T>>;

    template<typename T>
    struct InvokeSigsT : meta::TypeConstant<meta::List<ComplSig<T>>> {};

    template<concepts::Expected T>
    struct InvokeSigsT<T>
        : meta::TypeConstant<meta::List<ComplSig<meta::ExpectedValue<T>>, SetError(meta::ExpectedError<T>)>> {};

    template<typename T>
    using InvokeSigs = meta::Type<InvokeSigsT<T>>;

    template<typename CPO, typename Send, typename Env, typename Fun>
    using Sigs = meta::AsTemplate<
        CompletionSignatures,
        meta::Concat<PassthroughSignatures<CPO, meta::CompletionSignaturesOf<Send, MakeEnv<Env>>>,
                     meta::Join<meta::Transform<ResultTypes<CPO, Fun, meta::CompletionSignaturesOf<Send, MakeEnv<Env>>>,
                                                meta::Quote<InvokeSigs>>>>>;

    template<typename Fun, typename Rec>
    struct DataT {
        struct Type {
#ifdef DI_CLANG
            // NOTE: Clang for some reason crashes when using [[no_unique_address]] in debug mode here. In release mode,
            // the program itself segfaults...
            Fun function;
            Rec receiver;
#else
            [[no_unique_address]] Fun function;
            [[no_unique_address]] Rec receiver;
#endif
        };
    };

    template<typename Fun, typename Rec>
    using Data = meta::Type<DataT<meta::Decay<Fun>, Rec>>;

    template<typename CPO, typename Fun, typename Rec>
    struct ReceiverT {
        struct Type {
            using is_receiver = void;

            Data<Fun, Rec>* data;

            template<concepts::SameAs<CPO> Tg, typename... Args>
            requires(sizeof...(Args) < 2 && concepts::Invocable<Fun, Args...> &&
                     concepts::Expected<meta::InvokeResult<Fun, Args...>> &&
                     concepts::ReceiverOf<
                         Rec, meta::AsTemplate<CompletionSignatures, InvokeSigs<meta::InvokeResult<Fun, Args...>>>>)
            friend void tag_invoke(Tg, Type&& self, Args&&... args) {
                using R = meta::InvokeResult<Fun, Args...>;

                auto& data = *self.data;
                auto result = function::invoke(util::move(data.function), util::forward<Args>(args)...);
                if (!result) {
                    execution::set_error(util::move(data.receiver), util::move(result).error());
                } else {
                    if constexpr (concepts::LanguageVoid<meta::ExpectedValue<R>>) {
                        execution::set_value(util::move(data.receiver));
                    } else {
                        execution::set_value(util::move(data.receiver), util::move(result).value());
                    }
                }
            }

            template<concepts::SameAs<CPO> Tg, typename... Args>
            friend void tag_invoke(Tg, Type&& self, Args&&... args)
            requires(concepts::Invocable<Fun, Args...> && !concepts::Expected<meta::InvokeResult<Fun, Args...>> &&
                     concepts::ReceiverOf<Rec, CompletionSignatures<ComplSig<meta::InvokeResult<Fun, Args...>>>>)
            {
                using R = meta::InvokeResult<Fun, Args...>;

                if constexpr (concepts::LanguageVoid<R>) {
                    function::invoke(util::move(self.data->function), util::forward<Args>(args)...);
                    execution::set_value(util::move(self.data->receiver));
                } else {
                    execution::set_value(
                        util::move(self.data->receiver),
                        function::invoke(util::move(self.data->function), util::forward<Args>(args)...));
                }
            }

            template<concepts::OneOf<SetValue, SetError, SetStopped> Tg, typename... Args>
            friend void tag_invoke(Tg const tag, Type&& self, Args&&... args)
            requires(sizeof...(Args) < 2 && !concepts::SameAs<CPO, Tg> && concepts::Invocable<Tg, Rec, Args...>)
            {
                tag(util::move(self.data->receiver), util::forward<Args>(args)...);
            }

            friend auto tag_invoke(Tag<get_env>, Type const& self) { return make_env(get_env(self.data->receiver)); }
        };
    };

    template<typename CPO, typename Fun, typename Rec>
    using Receiver = meta::Type<ReceiverT<CPO, meta::Decay<Fun>, Rec>>;

    template<typename CPO, typename Send, typename Fun, typename Rec>
    struct OperationStateT {
        struct Type : util::Immovable {
        private:
            using Rc = Receiver<CPO, Fun, Rec>;
            using Op = meta::ConnectResult<Send, Rc>;

        public:
            explicit Type(Send&& sender, Fun&& function, Rec receiver)
                : m_data(util::forward<Fun>(function), util::move(receiver))
                , m_operation(connect(util::forward<Send>(sender), Rc(util::addressof(m_data)))) {}

        private:
            friend void tag_invoke(Tag<start>, Type& self) { start(self.m_operation); }

            Data<Fun, Rec> m_data;
            Op m_operation;
        };
    };

    template<typename CPO, typename Send, typename Fun, typename Rec>
    using OperationState = meta::Type<OperationStateT<CPO, Send, Fun, Rec>>;

    template<typename CPO, typename Send, typename Fun>
    struct SenderT {
        struct Type {
            using is_sender = void;

            [[no_unique_address]] Send sender;
            [[no_unique_address]] Fun function;

            template<concepts::RemoveCVRefSameAs<Type> Self, typename E>
            requires(concepts::DecayConvertible<meta::Like<Self, Fun>>)
            friend auto tag_invoke(Tag<get_completion_signatures>, Self&&, E&&)
                -> Sigs<CPO, meta::Like<Self, Send>, E, Fun>;

            template<concepts::RemoveCVRefSameAs<Type> Self, typename Rec>
            requires(concepts::DecayConvertible<meta::Like<Self, Fun>> &&
                     concepts::ReceiverOf<Rec, Sigs<CPO, meta::Like<Self, Send>, meta::EnvOf<Rec>, Fun>>)
            friend auto tag_invoke(Tag<connect>, Self&& self, Rec receiver) {
                return OperationState<CPO, meta::Like<Self, Send>, meta::Like<Self, Fun>, Rec>(
                    util::forward<Self>(self).sender, util::forward<Self>(self).function, util::move(receiver));
            }

            friend auto tag_invoke(Tag<get_env>, Type const& self) { return make_env(get_env(self.sender)); }
        };
    };

    template<typename CPO, typename Send, typename Fun>
    using Sender = meta::Type<SenderT<CPO, meta::RemoveCVRef<Send>, meta::Decay<Fun>>>;

    struct ValueFunction {
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
                return Sender<SetValue, Send, Fun> { util::forward<Send>(sender), util::forward<Fun>(function) };
            }
        }
    };

    struct ErrorFunction {
        template<concepts::Sender Send, concepts::MovableValue Fun>
        concepts::Sender auto operator()(Send&& sender, Fun&& function) const {
            if constexpr (requires {
                              function::tag_invoke(*this, get_completion_scheduler<SetError>(get_env(sender)),
                                                   util::forward<Send>(sender), util::forward<Fun>(function));
                          }) {
                return function::tag_invoke(*this, get_completion_scheduler<SetError>(get_env(sender)),
                                            util::forward<Send>(sender), util::forward<Fun>(function));
            } else if constexpr (requires {
                                     function::tag_invoke(*this, util::forward<Send>(sender),
                                                          util::forward<Fun>(function));
                                 }) {
                return function::tag_invoke(*this, util::forward<Send>(sender), util::forward<Fun>(function));
            } else {
                return Sender<SetError, Send, Fun> { util::forward<Send>(sender), util::forward<Fun>(function) };
            }
        }
    };

    struct StoppedFunction {
        template<concepts::Sender Send, concepts::MovableValue Fun>
        concepts::Sender auto operator()(Send&& sender, Fun&& function) const {
            if constexpr (requires {
                              function::tag_invoke(*this, get_completion_scheduler<SetStopped>(get_env(sender)),
                                                   util::forward<Send>(sender), util::forward<Fun>(function));
                          }) {
                return function::tag_invoke(*this, get_completion_scheduler<SetStopped>(get_env(sender)),
                                            util::forward<Send>(sender), util::forward<Fun>(function));
            } else if constexpr (requires {
                                     function::tag_invoke(*this, util::forward<Send>(sender),
                                                          util::forward<Fun>(function));
                                 }) {
                return function::tag_invoke(*this, util::forward<Send>(sender), util::forward<Fun>(function));
            } else {
                return Sender<SetStopped, Send, Fun> { util::forward<Send>(sender), util::forward<Fun>(function) };
            }
        }
    };
}

/// @brief A sender that maps values into another value.
///
/// @param sender The sender to map.
/// @param function The function to map the value with.
///
/// @returns A sender that maps values into another value.
///
/// This function synchronously maps values into another value. Additionally, the function can return an di::Expected,
/// which will be mapped into a value or an error.
///
/// If the transformation function wants to be asynchronous, use execution::let_value() instead, which allows the
/// function to return a sender.
///
/// The following examples show how to use this function:
/// @snippet{trimleft} tests/test_execution.cpp then
///
/// @see let_value
constexpr inline auto then = function::curry_back(then_ns::ValueFunction {}, c_<2zu>);

/// @brief A sender that maps an error into a value.
///
/// @param sender The sender to map.
/// @param function The function to map the error with.
///
/// @returns A sender that maps an error into a value.
///
/// This function is like execution::then(), but instead of mapping values into another value, it maps an error into a
/// value. Additionally, the function can return an di::Expected, which will be mapped into a value or an error.
///
/// @see then
constexpr inline auto upon_error = function::curry_back(then_ns::ErrorFunction {}, c_<2zu>);

/// @brief A sender that maps the stop signal into a value.
///
/// @param sender The sender to map.
/// @param function The function to map the stop signal with.
///
/// @returns A sender that maps the stop signal into a value.
///
/// This function is like execution::then(), but instead of mapping values into another value, it maps the stop signal
/// into a value. Additionally, the function can return an di::Expected, which will be mapped into a value or an error.
///
/// @see then
constexpr inline auto upon_stopped = function::curry_back(then_ns::StoppedFunction {}, c_<2zu>);
}

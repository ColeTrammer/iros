#pragma once

#include <di/any/concepts/any_storable_infallibly.h>
#include <di/any/concepts/any_storage.h>
#include <di/any/concepts/method.h>
#include <di/any/container/prelude.h>
#include <di/any/storage/hybrid_storage.h>
#include <di/any/types/method.h>
#include <di/any/types/this.h>
#include <di/execution/algorithm/just.h>
#include <di/execution/algorithm/just_or_error.h>
#include <di/execution/any/any_env.h>
#include <di/execution/any/any_operation_state.h>
#include <di/execution/any/any_receiver.h>
#include <di/execution/concepts/operation_state.h>
#include <di/execution/concepts/receiver_of.h>
#include <di/execution/concepts/sender.h>
#include <di/execution/concepts/sender_to.h>
#include <di/execution/concepts/valid_completion_signatures.h>
#include <di/execution/coroutine/lazy.h>
#include <di/execution/interface/connect.h>
#include <di/execution/meta/connect_result.h>
#include <di/execution/receiver/set_error.h>
#include <di/execution/receiver/set_stopped.h>
#include <di/execution/receiver/set_value.h>
#include <di/execution/types/completion_signuatures.h>
#include <di/function/tag_invoke.h>
#include <di/meta/algorithm.h>
#include <di/meta/core.h>
#include <di/meta/language.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/types/in_place_type.h>
#include <di/util/declval.h>
#include <di/util/defer_construct.h>
#include <di/util/move.h>
#include <di/vocab/error/error.h>
#include <di/vocab/expected/expected_forward_declaration.h>
#include <di/vocab/expected/unexpected.h>

namespace di::execution {
namespace detail {
    template<typename Rec, typename Op, typename Env>
    using AnySenderInterface = meta::List<types::Method<detail::ConnectFunction, Op(types::This&&, Rec)>>;
}

template<typename Sigs, typename Env, typename Storage, typename VTablePolicy, typename Op, typename Rec>
struct AnySenderT {
    class Type : public Any<detail::AnySenderInterface<Rec, Op, Env>, Storage, VTablePolicy> {
        using Base = Any<detail::AnySenderInterface<Rec, Op, Env>, Storage, VTablePolicy>;

        static_assert(concepts::ReceiverOf<Rec, detail::AnySigs<Sigs>>,
                      "Receiver must be able to receive all completion signatures.");

    public:
        using is_sender = void;

        using OperationState = Op;
        using Receiver = Rec;

        using CompletionSignatures = detail::AnySigs<Sigs>;

        Type(Type const&) = delete;
        Type& operator=(Type const&) = delete;

        Type(Type&&) = default;
        Type& operator=(Type&&) = default;

        template<typename E>
        requires(concepts::ConstructibleFrom<vocab::Error, E>)
        Type(vocab::Unexpected<E>&& error) : Type(just_error(vocab::Error(util::move(error).error()))) {}

        template<typename S, typename T = meta::RemoveCVRef<S>>
        requires(!concepts::DerivedFrom<T, Type> && concepts::SenderTo<T, Rec> && concepts::ConstructibleFrom<T, S>)
        Type(S&& sender) {
            if constexpr (concepts::AnyStorableInfallibly<T, typename Base::AnyStorage>) {
                this->emplace(util::forward<S>(sender));
            } else {
                auto result = this->emplace(util::forward<S>(sender));
                if (!result) {
                    using ErrorSender = decltype(execution::just_error(util::declval<vocab::Error>()));

                    static_assert(concepts::AnyStorableInfallibly<ErrorSender, typename Base::AnyStorage>,
                                  "Type-erased sender must have a large enough internal capacity to store the "
                                  "error sender without allocating.");

                    this->emplace(execution::just_error(vocab::Error(util::move(result).error())));
                }
            }
        }
    };
};

template<concepts::ValidCompletionSignatures Sigs, typename Env = void,
         concepts::AnyStorage Storage = any::HybridStorage<>, typename VTablePolicy = any::MaybeInlineVTable<3>,
         concepts::OperationState Op = AnyOperationState<>, typename Rec = AnyReceiver<detail::AnySigs<Sigs>>>
using AnySender = meta::Type<AnySenderT<Sigs, Env, Storage, VTablePolicy, Op, Rec>>;

template<typename T>
struct AnySenderOfT {
    class Type
        : public AnySender<types::CompletionSignatures<meta::AsLanguageFunction<
              SetValue, meta::Conditional<concepts::LanguageVoid<T>, meta::List<>, meta::List<T>>>>> {
    private:
        using Base = AnySender<types::CompletionSignatures<meta::AsLanguageFunction<
            SetValue, meta::Conditional<concepts::LanguageVoid<T>, meta::List<>, meta::List<T>>>>>;

    public:
        using Base::Base;
        using Base::operator=;

        using promise_type = execution::Lazy<T>::promise_type;

        Type()
        requires(concepts::LanguageVoid<T>)
            : Base(execution::just()) {}

        template<typename U>
        requires(!concepts::RemoveCVRefSameAs<Type, U> && !concepts::Sender<U> && concepts::ConstructibleFrom<T, U>)
        Type(U&& value) : Base(execution::just(T(util::forward<U>(value)))) {}

        template<typename U, typename E>
        requires(concepts::ConstructibleFrom<T, U> && concepts::ConstructibleFrom<vocab::Error, E>)
        Type(vocab::Expected<U, E>&& value) : Base(execution::just_or_error(util::move(value))) {}
    };
};

template<typename T = void>
using AnySenderOf = meta::Type<AnySenderOfT<T>>;

namespace detail {
    template<concepts::Sender Send, concepts::Method M,
             concepts::Receiver Rec = meta::At<meta::AsList<meta::MethodSignature<M>>, 1>,
             concepts::OperationState R = meta::LanguageFunctionReturn<meta::MethodSignature<M>>>
    requires(concepts::SenderTo<Send, Rec>)
    R tag_invoke(detail::ConnectFunction, M, Send&& sender, meta::TypeIdentity<Rec&&> receiver) {
        auto operation_state = R();

        using Op = meta::ConnectResult<Send, Rec>;

        using Storage = typename R::AnyStorage;
        if constexpr (concepts::AnyStorableInfallibly<Op, Storage>) {
            operation_state.emplace(in_place_type<Op>, util::DeferConstruct([&] {
                                        return execution::connect(util::forward<Send>(sender),
                                                                  util::forward<Rec>(receiver));
                                    }));
        } else {
            auto result = operation_state.emplace(in_place_type<Op>, util::DeferConstruct([&] {
                                                      return execution::connect(util::forward<Send>(sender),
                                                                                util::forward<Rec>(receiver));
                                                  }));
            if (!result) {
                using ErrorSender = decltype(execution::just_error(util::declval<vocab::Error>()));
                using OpError = meta::ConnectResult<ErrorSender, Rec>;

                static_assert(concepts::AnyStorableInfallibly<OpError, Storage>,
                              "Type-erased operation state must have a large enough internal capacity to store the "
                              "error operation state without allocating.");

                operation_state.emplace(in_place_type<OpError>, util::DeferConstruct([&] {
                                            return execution::connect(
                                                execution::just_error(vocab::Error(util::move(result).error())),
                                                util::forward<Rec>(receiver));
                                        }));
            }
        }

        return operation_state;
    }
}
}

namespace di {
using execution::AnySender;
using execution::AnySenderOf;
}

#pragma once

#include <di/execution/algorithm/into_variant.h>
#include <di/execution/concepts/receiver.h>
#include <di/execution/concepts/sender_in.h>
#include <di/execution/concepts/sender_to.h>
#include <di/execution/interface/connect.h>
#include <di/execution/meta/decayed_tuple.h>
#include <di/execution/meta/env_of.h>
#include <di/execution/meta/value_types_of.h>
#include <di/execution/query/get_completion_signatures.h>
#include <di/execution/query/make_env.h>
#include <di/execution/receiver/receiver_adaptor.h>
#include <di/execution/receiver/set_value.h>
#include <di/execution/types/completion_signuatures.h>
#include <di/function/tag_invoke.h>
#include <di/meta/language.h>
#include <di/meta/util.h>
#include <di/platform/prelude.h>
#include <di/types/prelude.h>
#include <di/util/move.h>
#include <di/vocab/error/error.h>
#include <di/vocab/error/result.h>
#include <di/vocab/expected/unexpect.h>

namespace di::execution {
namespace into_result_ns {
    template<typename... Types>
    struct ResultTypeImplHelper : meta::TypeConstant<meta::DecayedTuple<Types...>> {};

    template<>
    struct ResultTypeImplHelper<> : meta::TypeConstant<void> {};

    template<typename T>
    struct ResultTypeImplHelper<T> : meta::TypeConstant<meta::Decay<T>> {};

    struct ResultTypeImpl {
        template<typename... Types>
        using Invoke = meta::Type<ResultTypeImplHelper<Types...>>;
    };

    template<typename... Types>
    struct ResultTypeConcatImplHelper {};

    template<>
    struct ResultTypeConcatImplHelper<> : meta::TypeConstant<void> {};

    template<typename T>
    struct ResultTypeConcatImplHelper<T> : meta::TypeConstant<meta::Decay<T>> {};

    struct ResultTypeConcatImpl {
        template<typename... Types>
        using Invoke = meta::Type<ResultTypeConcatImplHelper<Types...>>;
    };

    template<typename Env, concepts::SenderIn<Env> Send>
    using ResultType = vocab::Result<
        meta::ValueTypesOf<Send, Env, ResultTypeImpl::template Invoke, ResultTypeConcatImpl::template Invoke>>;

    template<typename Env, concepts::SenderIn<Env> Send>
    using WithVariantResultType = vocab::Result<into_variant_ns::IntoVariantType<Send, Env>>;

    template<typename Result, typename Rec>
    struct ReceiverT {
        struct Type : ReceiverAdaptor<Type, Rec> {
            using Base = ReceiverAdaptor<Type, Rec>;
            friend Base;

        public:
            using Base::Base;

        private:
            template<typename T>
            requires(concepts::ConstructibleFrom<Result, InPlace, T>)
            void set_value(T&& value) && {
                execution::set_value(util::move(*this).base(), Result(in_place, util::forward<T>(value)));
            }

            void set_value() &&
                requires(concepts::LanguageVoid<Result>) {
                    execution::set_value(util::move(*this).base(), vocab::Result<Result>());
                }

                void set_error(vocab::Error error) && {
                execution::set_value(util::move(*this).base(), Result(types::unexpect, util::move(error)));
            }

            void set_stopped() && {
                execution::set_value(util::move(*this).base(), Result(types::unexpect, BasicError::OperationCanceled));
            }
        };
    };

    template<typename Value, concepts::Receiver Rec>
    using Receiver = meta::Type<ReceiverT<Value, Rec>>;

    template<typename Send>
    struct SenderT {
        struct Type {
            using is_sender = void;

            [[no_unique_address]] Send sender;

        private:
            template<concepts::DecaysTo<Type> Self, typename Env>
            friend auto tag_invoke(types::Tag<get_completion_signatures>, Self&&, Env&&)
                -> types::CompletionSignatures<SetValue(ResultType<MakeEnv<Env>, meta::Like<Self, Send>>)> {
                return {};
            }

            template<concepts::DecaysTo<Type> Self, concepts::Receiver Rec,
                     typename Value = ResultType<MakeEnv<meta::EnvOf<Rec>>, meta::Like<Self, Send>>>
            requires(concepts::DecayConstructible<meta::Like<Self, Send>> &&
                     concepts::SenderTo<meta::Like<Self, Send>, Receiver<Value, Rec>>)
            friend auto tag_invoke(types::Tag<connect>, Self&& self, Rec receiver) {
                return execution::connect(util::forward<Self>(self).sender,
                                          Receiver<Value, Rec> { util::move(receiver) });
            }

            constexpr friend auto tag_invoke(types::Tag<get_env>, Type const& self) {
                return make_env(get_env(self.sender));
            }
        };
    };

    template<concepts::Sender Send>
    using Sender = meta::Type<SenderT<Send>>;

    struct Function : function::pipeline::EnablePipeline {
        template<concepts::Sender Send>
        requires(concepts::DecayConstructible<Send>)
        auto operator()(Send&& sender) const {
            return Sender<meta::Decay<Send>> { util::forward<Send>(sender) };
        }
    };

    struct VariantFunction : function::pipeline::EnablePipeline {
        template<concepts::Sender Send>
        requires(concepts::DecayConstructible<Send>)
        auto operator()(Send&& sender) const {
            return Function {}(execution::into_variant(util::forward<Send>(sender)));
        }
    };
}

constexpr inline auto into_result = into_result_ns::Function {};
constexpr inline auto into_variant_result = into_result_ns::VariantFunction {};
}

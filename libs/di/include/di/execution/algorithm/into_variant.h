#pragma once

#include "di/execution/concepts/prelude.h"
#include "di/execution/concepts/sender_in.h"
#include "di/execution/meta/decayed_tuple.h"
#include "di/execution/meta/prelude.h"
#include "di/execution/receiver/prelude.h"
#include "di/execution/types/prelude.h"
#include "di/function/pipeline.h"
#include "di/types/prelude.h"

namespace di::execution {
namespace into_variant_ns {
    template<typename Send, typename Env>
    requires(concepts::SenderIn<Send, Env>)
    using IntoVariantType = meta::ValueTypesOf<Send, Env>;

    template<typename Send, typename Env>
    struct IntoVariantSetValue : meta::TypeConstant<CompletionSignatures<SetValue(IntoVariantType<Send, Env>)>> {};

    template<typename Value, typename Rec>
    struct ReceiverT {
        struct Type : ReceiverAdaptor<Type, Rec> {
            using Base = ReceiverAdaptor<Type, Rec>;
            friend Base;

        public:
            using Base::Base;

        private:
            template<typename... Args>
            requires(concepts::ConstructibleFrom<Value, InPlaceType<meta::DecayedTuple<Args...>>, Args...>)
            void set_value(Args&&... args) && {
                execution::set_value(util::move(*this).base(), Value { in_place_type<meta::DecayedTuple<Args...>>,
                                                                       util::forward<Args>(args)... });
            }
        };
    };

    template<concepts::InstanceOf<Variant> Value, concepts::Receiver Rec>
    using Receiver = meta::Type<ReceiverT<Value, Rec>>;

    template<typename Send>
    struct SenderT {
        struct Type {
            using is_sender = void;

            [[no_unique_address]] Send sender;

        private:
            template<concepts::DecaysTo<Type> Self, typename Env>
            friend auto tag_invoke(types::Tag<get_completion_signatures>, Self&&, Env&&)
                -> meta::MakeCompletionSignatures<
                    meta::Like<Self, Send>, MakeEnv<Env>, CompletionSignatures<>,
                    IntoVariantSetValue<meta::Like<Self, Send>, MakeEnv<Env>>::template Invoke> {
                return {};
            }

            template<concepts::DecaysTo<Type> Self, concepts::Receiver Rec,
                     typename Value = IntoVariantType<meta::Like<Self, Send>, MakeEnv<meta::EnvOf<Rec>>>>
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
}

constexpr inline auto into_variant = into_variant_ns::Function {};
}

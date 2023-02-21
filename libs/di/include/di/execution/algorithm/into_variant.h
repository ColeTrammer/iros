#pragma once

#include <di/execution/concepts/prelude.h>
#include <di/execution/meta/prelude.h>
#include <di/execution/receiver/prelude.h>
#include <di/execution/types/prelude.h>
#include <di/function/pipeline.h>

namespace di::execution {
namespace into_variant_ns {
    template<typename Send, typename Env>
    requires(concepts::Sender<Send, Env>)
    using IntoVariantType = meta::ValueTypesOf<Send, Env>;

    template<typename Send, typename Env>
    struct IntoVariantSetValue : meta::Id<SetValue(IntoVariantType<Send, Env>)> {};

    template<typename Value, typename Rec>
    struct ReceiverT {
        struct Type : private ReceiverAdaptor<Type, Rec> {
            using Base = ReceiverAdaptor<Type, Rec>;
            friend Base;

        public:
            using Base::Base;

        private:
            template<typename... Args>
            requires(concepts::ConstructibleFrom<meta::DecayedTuple<Args...>, Args...>)
            void set_value(Args&&... args) && {
                using Tup = meta::DecayedTuple<Args...>;

                execution::set_value(util::move(*this), Value { in_place_type<Tup>, util::forward<Args>(args)... });
            }
        };
    };

    template<concepts::InstanceOf<Variant> Value, concepts::Receiver Rec>
    using Receiver = meta::Type<ReceiverT<Value, Rec>>;

    template<typename Send>
    struct SenderT {
        struct Type {
            [[no_unique_address]] Send sender;

        private:
            template<concepts::DecaysTo<Type> Self, typename Env>
            friend auto tag_invoke(types::Tag<get_completion_signatures>, Self&&, Env)
                -> meta::MakeCompletionSignatures<meta::Like<Self, Send>, Env, CompletionSignatures<>,
                                                  IntoVariantSetValue<meta::Like<Self, Send>, Env>::template Invoke>;

            template<concepts::DecaysTo<Type> Self, concepts::Receiver Rec,
                     typename Value = IntoVariantType<meta::Like<Self, Send>, meta::EnvOf<Rec>>>
            requires(concepts::DecayConstructible<meta::Like<Self, Send>> &&
                     concepts::SenderTo<meta::Like<Self, Send>, Receiver<Value, Rec>>)
            friend auto tag_invoke(types::Tag<connect>, Self&& self, Rec receiver) {
                return execution::connect(util::forward<Self>(self).sender,
                                          Receiver<Value, Rec> { util::move(receiver) });
            }

            template<concepts::ForwardingSenderQuery Tag, typename... Args>
            constexpr friend auto tag_invoke(Tag tag, Type const& self, Args&&... args)
                -> decltype(tag(self.sender, util::forward<Args>(args)...)) {
                return tag(self.sender, util::forward<Args>(args)...);
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

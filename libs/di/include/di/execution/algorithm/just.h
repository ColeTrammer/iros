#pragma once

#include <di/execution/concepts/prelude.h>
#include <di/execution/interface/start.h>
#include <di/execution/types/prelude.h>
#include <di/meta/util.h>
#include <di/vocab/tuple/prelude.h>

namespace di::execution {
namespace just_ns {
    template<typename CPO, typename Rec, typename... Types>
    struct OperationStateT {
        struct Type {
        public:
            [[no_unique_address]] Tuple<Types...> values;
            [[no_unique_address]] Rec receiver;

        private:
            constexpr friend void tag_invoke(types::Tag<execution::start>, Type& self) {
                apply(
                    [&](Types&... values) {
                        CPO {}(util::move(self.receiver), util::move(values)...);
                    },
                    self.values);
            }
        };
    };

    template<concepts::OneOf<SetValue, SetStopped, SetError> CPO, concepts::Receiver Rec, typename... Types>
    using OperationState = meta::Type<OperationStateT<CPO, Rec, Types...>>;

    struct ConstructTag {};

    constexpr inline auto construct_tag = ConstructTag {};

    template<typename CPO, typename... Types>
    struct SenderT {
        struct Type {
        public:
            using is_sender = void;

            using CompletionSignatures = types::CompletionSignatures<CPO(Types...)>;

            template<typename... Us>
            constexpr explicit Type(ConstructTag, Us&&... values_) : values(util::forward<Us>(values_)...) {}

            [[no_unique_address]] Tuple<Types...> values;

        private:
            template<concepts::ReceiverOf<CompletionSignatures> Rec>
            requires(concepts::CopyConstructible<Types> && ...)
            constexpr friend auto tag_invoke(types::Tag<execution::connect>, Type const& sender, Rec receiver) {
                return OperationState<CPO, Rec, Types...> { sender.values, util::move(receiver) };
            }

            template<concepts::ReceiverOf<CompletionSignatures> Rec>
            requires(concepts::MoveConstructible<Types> && ...)
            constexpr friend auto tag_invoke(types::Tag<execution::connect>, Type&& sender, Rec receiver) {
                return OperationState<CPO, Rec, Types...> { util::move(sender.values), util::move(receiver) };
            }
        };
    };

    template<concepts::OneOf<SetValue, SetStopped, SetError> CPO, typename... Types>
    using Sender = meta::Type<SenderT<CPO, meta::Decay<Types>...>>;

    struct Function {
        template<concepts::MovableValue... Values>
        constexpr auto operator()(Values&&... values) const -> concepts::Sender auto {
            return Sender<SetValue, Values...> { construct_tag, util::forward<Values>(values)... };
        }
    };

    struct ErrorFunction {
        template<concepts::MovableValue Error>
        constexpr auto operator()(Error&& error) const -> concepts::Sender auto {
            return Sender<SetError, Error> { construct_tag, util::forward<Error>(error) };
        }
    };

    struct StoppedFunction {
        constexpr auto operator()() const -> concepts::Sender auto { return Sender<SetStopped> { construct_tag }; }
    };
}

constexpr inline auto just = just_ns::Function {};
constexpr inline auto just_error = just_ns::ErrorFunction {};
constexpr inline auto just_stopped = just_ns::StoppedFunction {};

using Stopped = decltype(just_stopped());
constexpr inline auto stopped = just_stopped();
}

namespace di {
using execution::Stopped;
using execution::stopped;
}

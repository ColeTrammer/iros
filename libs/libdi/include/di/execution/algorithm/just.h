#pragma once

#include <di/concepts/movable_value.h>
#include <di/execution/concepts/prelude.h>
#include <di/execution/interface/start.h>
#include <di/execution/types/prelude.h>
#include <di/vocab/tuple/prelude.h>

namespace di::execution {
namespace detail {
    template<typename CPO, typename... Types>
    class JustSender {
    public:
        using CompletionSignatures = types::CompletionSignatures<CPO(Types...)>;

        [[no_unique_address]] Tuple<Types...> values;

    private:
        template<typename Rec>
        class OperationState {
        public:
            [[no_unique_address]] Tuple<Types...> values;
            [[no_unique_address]] Rec receiver;

        private:
            constexpr friend void tag_invoke(types::Tag<execution::start>, OperationState& state) {
                apply(
                    [&](Types&... values) {
                        CPO {}(util::move(state.receiver), util::move(values)...);
                    },
                    state.values);
            }
        };

    public:
    private:
        template<concepts::ReceiverOf<CompletionSignatures> Rec>
        requires(concepts::Conjunction<concepts::CopyConstructible<Types>...>)
        constexpr friend auto tag_invoke(types::Tag<execution::connect>, JustSender const& sender, Rec&& receiver) {
            return OperationState<meta::Decay<Rec>> { sender.values, util::forward<Rec>(receiver) };
        }

        template<concepts::ReceiverOf<CompletionSignatures> Rec>
        requires(concepts::Conjunction<concepts::CopyConstructible<Types>...>)
        constexpr friend auto tag_invoke(types::Tag<execution::connect>, JustSender&& sender, Rec&& receiver) {
            return OperationState<meta::Decay<Rec>> { util::move(sender.values), util::forward<Rec>(receiver) };
        }
    };

    struct JustFunction {
        template<concepts::MovableValue... Values>
        constexpr concepts::Sender auto operator()(Values&&... values) const {
            return JustSender<SetValue, meta::Decay<Values>...> { { util::forward<Values>(values)... } };
        }
    };

    struct JustErrorFunction {
        template<concepts::MovableValue Error>
        constexpr concepts::Sender auto operator()(Error&& error) const {
            return JustSender<SetError, meta::Decay<Error>> { { util::forward<Error>(error) } };
        }
    };

    struct JustStoppedFunction {
        constexpr concepts::Sender auto operator()() const { return JustSender<SetStopped> { {} }; }
    };
}

constexpr inline auto just = detail::JustFunction {};
constexpr inline auto just_error = detail::JustErrorFunction {};
constexpr inline auto just_stopped = detail::JustStoppedFunction {};
}
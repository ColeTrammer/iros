#pragma once

#include "di/execution/concepts/receiver.h"
#include "di/execution/concepts/receiver_of.h"
#include "di/execution/concepts/sender.h"
#include "di/execution/interface/connect.h"
#include "di/execution/interface/start.h"
#include "di/execution/receiver/set_error.h"
#include "di/execution/receiver/set_value.h"
#include "di/execution/types/completion_signuatures.h"
#include "di/function/tag_invoke.h"
#include "di/meta/algorithm.h"
#include "di/meta/core.h"
#include "di/meta/language.h"
#include "di/meta/operations.h"
#include "di/meta/vocab.h"
#include "di/util/move.h"
#include "di/vocab/error/error.h"

namespace di::execution {
namespace just_or_error_ns {
    template<typename R, typename Rec>
    struct OperationStateT {
        struct Type : util::Immovable {
        public:
            explicit Type(R&& value_, Rec receiver_) : value(util::move(value_)), receiver(util::move(receiver_)) {}

            [[no_unique_address]] R value;
            [[no_unique_address]] Rec receiver;

        private:
            constexpr friend void tag_invoke(types::Tag<di::execution::start>, Type& self) {
                if (!self.value.has_value()) {
                    return execution::set_error(util::move(self.receiver),
                                                vocab::Error(util::move(self.value).error()));
                }
                if constexpr (concepts::LanguageVoid<meta::ExpectedValue<R>>) {
                    return execution::set_value(util::move(self.receiver));
                } else {
                    return execution::set_value(util::move(self.receiver), util::move(self.value).value());
                }
            }
        };
    };

    template<concepts::Expected R, concepts::Receiver Rec>
    using OperationState = meta::Type<OperationStateT<R, Rec>>;

    template<typename R>
    struct SenderT {
        struct Type {
        public:
            using is_sender = void;

            using CompletionSignatures = types::CompletionSignatures<
                meta::AsLanguageFunction<SetValue, meta::Conditional<concepts::LanguageVoid<meta::ExpectedValue<R>>,
                                                                     meta::List<>, meta::List<meta::ExpectedValue<R>>>>,
                SetError(vocab::Error)>;

            [[no_unique_address]] R value;

        private:
            template<concepts::ReceiverOf<CompletionSignatures> Rec>
            requires(concepts::CopyConstructible<R>)
            constexpr friend auto tag_invoke(types::Tag<di::execution::connect>, Type const& self, Rec receiver) {
                return OperationState<R, Rec> { self.value, util::move(receiver) };
            }

            template<concepts::ReceiverOf<CompletionSignatures> Rec>
            constexpr friend auto tag_invoke(types::Tag<di::execution::connect>, Type&& self, Rec receiver) {
                return OperationState<R, Rec> { util::move(self.value), util::move(receiver) };
            }
        };
    };

    template<concepts::Expected R>
    using Sender = meta::Type<SenderT<R>>;

    struct Function {
        auto operator()(concepts::Expected auto value) const -> concepts::Sender auto {
            return Sender<decltype(value)> { util::move(value) };
        }
    };
}

constexpr inline auto just_or_error = just_or_error_ns::Function {};
}

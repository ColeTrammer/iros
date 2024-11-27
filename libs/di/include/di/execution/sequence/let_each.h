#pragma once

#include <di/execution/algorithm/let.h>
#include <di/execution/concepts/sender.h>
#include <di/execution/sequence/sequence_sender.h>
#include <di/execution/sequence/transform_each.h>
#include <di/function/curry_back.h>
#include <di/function/tag_invoke.h>
#include <di/meta/util.h>

namespace di::execution {
namespace let_each_ns {
    struct ValueFunction {
        template<concepts::Sender Seq, concepts::MovableValue Fun>
        auto operator()(Seq&& sequence, Fun&& transformer) const -> concepts::SequenceSender auto {
            if constexpr (concepts::TagInvocable<ValueFunction, Seq, Fun>) {
                return function::tag_invoke(*this, util::forward<Seq>(sequence), util::forward<Fun>(transformer));
            } else {
                return transform_each(util::forward<Seq>(sequence), let_value(util::forward<Fun>(transformer)));
            }
        }
    };

    struct ErrorFunction {
        template<concepts::Sender Seq, concepts::MovableValue Fun>
        auto operator()(Seq&& sequence, Fun&& transformer) const -> concepts::SequenceSender auto {
            if constexpr (concepts::TagInvocable<ErrorFunction, Seq, Fun>) {
                return function::tag_invoke(*this, util::forward<Seq>(sequence), util::forward<Fun>(transformer));
            } else {
                return transform_each(util::forward<Seq>(sequence), let_error(util::forward<Fun>(transformer)));
            }
        }
    };

    struct StoppedFunction {
        template<concepts::Sender Seq, concepts::MovableValue Fun>
        auto operator()(Seq&& sequence, Fun&& transformer) const -> concepts::SequenceSender auto {
            if constexpr (concepts::TagInvocable<StoppedFunction, Seq, Fun>) {
                return function::tag_invoke(*this, util::forward<Seq>(sequence), util::forward<Fun>(transformer));
            } else {
                return transform_each(util::forward<Seq>(sequence), let_stopped(util::forward<Fun>(transformer)));
            }
        }
    };
}

/// @brief Transform the values of a sequence into new senders.
///
/// @param sequence The sequence to transform.
/// @param transformer The function to transform the values, which returns a sender.
///
/// @returns A sequence sender that transforms the values of the sequence into new senders.
///
/// @note Like execution::let_value, the transformer function is passed lvalues internally stored in the operation
/// state. This means they are safe to capture by reference in the transformer function.
///
/// @see let_value
/// @see transform_each
constexpr inline auto let_value_each = function::curry_back(let_each_ns::ValueFunction {}, meta::c_<2zu>);

/// @brief Transform the errors of a sequence into new senders.
///
/// @param sequence The sequence to transform.
/// @param transformer The function to transform the errors, which returns a sender.
///
/// @returns A sequence sender that transforms the errors of the sequence into new senders.
///
/// This function allows mapping the errors of a sequence into values, which can be used to make the sequence complete
/// successfully.
///
/// @note Like execution::let_error, the transformer function is passed lvalues internally stored in the operation
/// state. This means they are safe to capture by reference in the transformer function.
///
/// @see let_error
/// @see transform_each
constexpr inline auto let_error_each = function::curry_back(let_each_ns::ErrorFunction {}, meta::c_<2zu>);

/// @brief Transform the stopped signal of a sequence into new senders.
///
/// @param sequence The sequence to transform.
/// @param transformer The function to transform the stopped signal, which returns a sender.
///
/// @returns A sequence sender that transforms the stopped signal of the sequence into new senders.
///
/// This function allows mapping the stopped signal of a sequence into values, which can be used to make the sequence
/// never stop.
///
/// @see let_stopped
/// @see transform_each
constexpr inline auto let_stopped_each = function::curry_back(let_each_ns::StoppedFunction {}, meta::c_<2zu>);
}

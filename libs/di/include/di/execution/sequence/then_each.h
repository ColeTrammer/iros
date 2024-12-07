#pragma once

#include "di/execution/algorithm/then.h"
#include "di/execution/sequence/transform_each.h"

namespace di::execution {
namespace then_each_ns {
    struct Function {
        template<concepts::Sender Seq, concepts::MovableValue Fun>
        auto operator()(Seq&& sequence, Fun&& transformer) const -> concepts::SequenceSender auto {
            if constexpr (concepts::TagInvocable<Function, Seq, Fun>) {
                return function::tag_invoke(*this, util::forward<Seq>(sequence), util::forward<Fun>(transformer));
            } else {
                return transform_each(util::forward<Seq>(sequence), then(util::forward<Fun>(transformer)));
            }
        }
    };

    struct ErrorFunction {
        template<concepts::Sender Seq, concepts::MovableValue Fun>
        auto operator()(Seq&& sequence, Fun&& transformer) const -> concepts::SequenceSender auto {
            if constexpr (concepts::TagInvocable<Function, Seq, Fun>) {
                return function::tag_invoke(*this, util::forward<Seq>(sequence), util::forward<Fun>(transformer));
            } else {
                return transform_each(util::forward<Seq>(sequence), upon_error(util::forward<Fun>(transformer)));
            }
        }
    };

    struct StoppedFunction {
        template<concepts::Sender Seq, concepts::MovableValue Fun>
        auto operator()(Seq&& sequence, Fun&& transformer) const -> concepts::SequenceSender auto {
            if constexpr (concepts::TagInvocable<Function, Seq, Fun>) {
                return function::tag_invoke(*this, util::forward<Seq>(sequence), util::forward<Fun>(transformer));
            } else {
                return transform_each(util::forward<Seq>(sequence), upon_stopped(util::forward<Fun>(transformer)));
            }
        }
    };
}

/// @brief Transform the values of a sequence directly.
///
/// @param sequence The sequence to transform.
/// @param transformer The function to transform the values.
///
/// @returns A sequence sender that transforms the values of the sequence directly.
///
/// This function is the equivalent of view::transform(), but for sequences. However, the transformer is
/// allowed to return a vocab::Result<T> instead of a T, which will propogate the error out of the sequence.
///
/// @note Like execution::then, the transformer function is passed values directly, so parameters should be taken by
/// value.
///
/// @see then
/// @see transform_each
/// @see view::transform
constexpr inline auto then_each = function::curry_back(then_each_ns::Function {}, meta::c_<2ZU>);

/// @brief Transform the errors of a sequence directly.
///
/// @param sequence The sequence to transform.
/// @param transformer The function to transform the errors.
///
/// @returns A sequence sender that transforms the errors of the sequence directly.
///
/// This function is execution::then_each(), but maps errors instead of values. The transformer is allowed to
/// return a vocab::Result<T> instead of a T, which will propogate the error out of the sequence.
///
/// @note Like execution::upon_error, the transformer function is passed errors directly, so parameters should be
/// taken by value.
///
/// @see upon_error
/// @see then_each
/// @see transform_each
constexpr inline auto upon_error_each = function::curry_back(then_each_ns::ErrorFunction {}, meta::c_<2ZU>);

/// @brief Transform stopped values of a sequence directly.
///
/// @param sequence The sequence to transform.
/// @param transformer The function to transform the stopped values.
///
/// @returns A sequence sender that transforms the stopped values of the sequence directly.
///
/// This function is execution::then_each(), but runs when the sequence sends stopped instead of values. The transformer
/// is allowed to return a vocab::Result<T> instead of a T, which will propogate the error out of the sequence.
///
/// @see upon_stopped
/// @see then_each
/// @see transform_each
constexpr inline auto upon_stopped_each = function::curry_back(then_each_ns::StoppedFunction {}, meta::c_<2ZU>);
}

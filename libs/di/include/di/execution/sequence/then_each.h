#pragma once

#include <di/execution/algorithm/then.h>
#include <di/execution/sequence/transform_each.h>

namespace di::execution {
namespace then_each_ns {
    struct Function {
        template<concepts::Sender Seq, concepts::MovableValue Fun>
        concepts::SequenceSender auto operator()(Seq&& sequence, Fun&& transformer) const {
            if constexpr (concepts::TagInvocable<Function, Seq, Fun>) {
                return function::tag_invoke(*this, util::forward<Seq>(sequence), util::forward<Fun>(transformer));
            } else {
                return transform_each(util::forward<Seq>(sequence), then(util::forward<Fun>(transformer)));
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
constexpr inline auto then_each = function::curry_back(then_each_ns::Function {}, meta::size_constant<2>);
}

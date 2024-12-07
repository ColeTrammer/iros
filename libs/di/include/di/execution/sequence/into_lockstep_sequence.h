#pragma once

#include "di/execution/concepts/sender.h"
#include "di/execution/query/is_always_lockstep_sequence.h"
#include "di/function/pipeable.h"
#include "di/function/tag_invoke.h"
#include "di/meta/util.h"

namespace di::execution {
namespace into_lockstep_sequence_ns {
    struct Function : function::pipeline::EnablePipeline {
        template<concepts::Sender Seq>
        auto operator()(Seq&& sequence) const {
            if constexpr (concepts::AlwaysLockstepSequence<Seq>) {
                return util::forward<Seq>(sequence);
            } else if constexpr (concepts::TagInvocable<Function, Seq>) {
                using Result = meta::TagInvokeResult<Function, Seq>;
                static_assert(
                    concepts::AlwaysLockstepSequence<Result>,
                    "into_lockstep_sequence() customizations must return a sequence that is always lockstep.");
                return tag_invoke(*this, util::forward<Seq>(sequence));
            } else {
                static_assert(concepts::AlwaysFalse<Seq>,
                              "FIXME: into_lockstep_sequence() does not have yet a default implementation.");
                return util::forward<Seq>(sequence);
            }
        }
    };
}

/// @brief Converts a sequence into a lockstep sequence.
///
/// @param sequence The sequence to convert.
///
/// @return A lockstep sequence.
///
/// This function is a no-op for sequences that are already lockstep. For non-lockstep sequences, this function will
/// return a lockstep sequence, hopefully by using an atomic instrusive queue of operation states, but this is not yet
/// implemented.
///
/// This function is useful for sequence algorithms which require lockstep sequences, like execution::zip and
/// execution::fold, or any algorithm which cannot process results in parallel (which includes consuming sequences in
/// a coroutine).
///
/// @see execution::is_always_lockstep_sequence
/// @see concepts::AlwaysLockstepSequence
constexpr inline auto into_lockstep_sequence = into_lockstep_sequence_ns::Function {};
}

namespace di::meta {
/// @brief Deduce the type of converting a sequence into a lockstep sequence.
///
/// @tparam Seq The sequence to convert.
///
/// @see execution::into_lockstep_sequence
template<concepts::Sender Seq>
using IntoLockstepSequence = decltype(execution::into_lockstep_sequence(util::declval<Seq>()));
}

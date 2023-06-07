#pragma once

#include <di/execution/meta/env_of.h>
#include <di/execution/query/forwarding_query.h>
#include <di/execution/sequence/sequence_sender.h>
#include <di/function/invoke.h>
#include <di/function/tag_invoke.h>
#include <di/meta/constexpr.h>

namespace di::execution {
namespace is_always_lockstop_sequence_ns {
    struct Function : ForwardingQuery {
        template<typename Env>
        constexpr auto operator()(Env const& env) const {
            if constexpr (concepts::TagInvocable<Function, Env const&>) {
                static_assert(
                    concepts::ConstexprOf<meta::TagInvokeResult<Function, Env const&>, bool>,
                    "is_always_lockstep_sequence() customizations must return a di::Constexpr<bool> instance.");
                return tag_invoke(*this, env);
            } else {
                return c_<false>;
            }
        }
    };
}

/// @brief A query that returns whether or not a sequence is always lockstep.
///
/// @param env The sequence's environment.
///
/// @return A `di::Constexpr<bool>` instance.
///
/// This query allows optimizing the execution of a sequence by guaranteeing that it does not ever make concurrent calls
/// to `execution::set_next()`. In particular, another call to `execution::set_next()` can only be made once the sender
/// returned by the previous call to `execution::set_next()` has completed.
///
/// This is true for sequences such as those that are created by `execution::AsyncGenerator` or
/// `execution::from_container`. This is also true for most sequence algorithms which operate on lockstep sequences, and
/// for algorithms which must transform input sequences to be lockstep (like execution::zip and execution::fold).
/// Additionally, this is true for sequences which only send a single value, including regular senders. However, since
/// this is used as an optimization, it defaults to false, since falsely returning true will most likely will result in
/// undefined behavior.
///
/// This property should be queried using the `concepts::AlwaysLockstepSequence` concept, which will return true if the
/// sequence is a regular sender or a lockstep sequence.
///
/// @note This query must return a `di::Constexpr<bool>` instance, so if this property varies at run-time (like for a
/// type-erased sequence), it must return false.
///
/// @see concepts::AlwaysLockstepSequence
constexpr inline auto is_always_lockstep_sequence = is_always_lockstop_sequence_ns::Function {};
}

namespace di::concepts {
/// @brief Checks if `Send` is a sequence that is always lockstep.
///
/// @tparam Send The type to check.
///
/// @see execution::is_always_lockstep_sequence
template<typename Send>
concept AlwaysLockstepSequence =
    !concepts::SequenceSender<Send> ||
    (meta::InvokeResult<Tag<execution::is_always_lockstep_sequence>, meta::EnvOf<Send> const&>::value);
}

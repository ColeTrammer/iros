#pragma once

#include <di/execution/meta/env_of.h>
#include <di/execution/query/forwarding_query.h>
#include <di/execution/sequence/sequence_sender.h>
#include <di/function/invoke.h>
#include <di/function/tag_invoke.h>
#include <di/math/numeric_limits.h>
#include <di/meta/constexpr.h>
#include <di/types/integers.h>

namespace di::execution {
namespace get_sequence_cardinality_ns {
    struct Function : ForwardingQuery {
        template<typename Env>
        constexpr auto operator()(Env const& env) const {
            if constexpr (concepts::TagInvocable<Function, Env const&>) {
                static_assert(concepts::ConstexprOf<meta::TagInvokeResult<Function, Env const&>, usize>,
                              "sequence_cardinality() customizations must return a di::Constexpr<usize> instance.");
                return tag_invoke(*this, env);
            } else {
                return c_<math::NumericLimits<usize>::max>;
            }
        }
    };
}

/// @brief A query that returns the cardinality of a sequence.
///
/// @param env The sequence's environment.
///
/// @return A `di::Constexpr<usize>` instance.
///
/// This query allows optimizing the execution of a sequence by having a static upper bound on the number of values it
/// will send. This could be used to know how many values to pre-allocate for a sequence, or to verify that a sequence
/// only sends a single value (for an async resource).
///
/// This property should be queried using the `meta::SequenceCardinality` template, which will return the cardinality
/// of a sequence, or 1 in the case of a regular sender.
///
/// @note This query must return a `di::Constexpr<usize>` instance, so if this property varies at run-time (like for a
/// type-erased sequence), it should return `math::NumericLimits<usize>::max`.
///
/// @see meta::SequenceCardinality
constexpr inline auto get_sequence_cardinality = get_sequence_cardinality_ns::Function {};
}

namespace di::meta {
/// @brief Gets the cardinality of a sequence.
///
/// @tparam Sequence The sequence to get the cardinality of.
///
/// @see execution::get_sequence_cardinality
template<typename Sequence>
constexpr inline auto SequenceCardinality =
    !concepts::SequenceSender<Sequence>
        ? 1zu
        : meta::InvokeResult<decltype(execution::get_sequence_cardinality), meta::EnvOf<Sequence>>::value;
}

#pragma once

#include "di/execution/concepts/sender.h"
#include "di/execution/query/get_sequence_cardinality.h"
#include "di/execution/sequence/sequence_sender.h"
#include "di/function/tag_invoke.h"

namespace di::execution {
namespace run_ns {
    struct Function {
        template<typename T>
        requires(concepts::TagInvocable<Function, T&>)
        auto operator()(T& resource) const {
            using R = meta::TagInvokeResult<Function, T&>;
            static_assert(concepts::Sender<R>, "run() customizations must return a SequenceSender.");
            static_assert(meta::SequenceCardinality<R> == 1,
                          "run() customizations must return a SequenceSender with cardinality 1.");
            return tag_invoke(*this, resource);
        }
    };
}

/// @brief Obtain access to an async resource.
///
/// @param resource The resource to access.
///
/// @returns A sequence sender that can be used to access the resource.
///
/// This function is an internal CPO which is used by async resources. This function is not intended to be called
/// directly. Resources should instead be consumed by using the execution::use_resources function.
///
/// Custom async resources must override this function to provide access to the resource. The returned sequence must
/// send exactly one value and should complete when this value is accepted. The sent value should be some sort of token
/// is used to access the resource. After the value is accepted, cleanup of the resource should be started if needed.
/// Sending the value or completing can send errors if it makes sense to do so.
///
/// See the @subpage md_docs_2di_2execution document for more information on the async resource model.
///
/// @see use_resources
constexpr inline auto run = run_ns::Function {};
}

namespace di::concepts {
/// @brief Checks that a type model AsyncResource.
///
/// @tparam T The type to check.
///
/// @see execution::run
template<typename T>
concept AsyncResource = requires(T& resource) {
    { execution::run(resource) };
};
}

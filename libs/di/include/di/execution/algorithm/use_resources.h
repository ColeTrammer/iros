#pragma once

#include <di/execution/algorithm/let_value_with.h>
#include <di/execution/concepts/sender.h>
#include <di/execution/interface/run.h>
#include <di/execution/sequence/first_value.h>
#include <di/execution/sequence/let_each.h>
#include <di/execution/sequence/zip.h>
#include <di/function/invoke.h>
#include <di/function/tag_invoke.h>
#include <di/meta/util.h>
#include <di/util/move.h>

namespace di::execution {
namespace use_resources_ns {
    struct Function {
        template<concepts::MovableValue Fun, concepts::MovableValue... Deferred>
        requires(concepts::AsyncResource<meta::InvokeResult<meta::Decay<Deferred> &&>> && ...)
        auto operator()(Fun&& function, Deferred&&... deferred) const {
            if constexpr (concepts::TagInvocable<Function, Fun, Deferred...>) {
                static_assert(concepts::Sender<meta::TagInvokeResult<Function, Fun, Deferred...>>,
                              "use_resources() customizations must return a Sender.");
                return function::tag_invoke(*this, util::forward<Fun>(function), util::forward<Deferred>(deferred)...);
            } else {
                return execution::let_value_with(
                    [function = util::forward<Fun>(function)](auto&... resources) {
                        return first_value(zip(run(resources)...) | let_value_each(util::move(function)));
                    },
                    util::forward<Deferred>(deferred)...);
            }
        }
    };
}

/// @brief Use async resources.
///
/// @param function The function which accepts the resources.
/// @param deferred Possibly multiple factories which return async resources.
///
/// @returns A sender which runs the sender returned from the function.
///
/// This function allows consuming multiple async resources in parallel. If any resource fails to be acquired, the
/// returned sender sends an error. Likewise, if any cleanup fails, the returned sender sends an error. The provided
/// function is only invoked if all resources are successfully acquired. The values sent by the function are forwarded
/// to the returned sender.
///
/// The values passed to the function are the async resource tokens. These are regular types which internally have
/// reference semantics. They refer to the actual resouce objects, which are stored in an internal operation state. This
/// function is the recommended way to consume async resources, since it guarantees that the resources are acquired
/// and released, and even does so in parallel.
///
/// See the following usage example:
/// @snippet{trimleft} test/test_execution.cpp use_resources
///
/// @see run
constexpr inline auto use_resources = use_resources_ns::Function {};
}

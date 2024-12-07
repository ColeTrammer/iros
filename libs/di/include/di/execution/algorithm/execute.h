#pragma once

#include "di/container/allocator/allocator.h"
#include "di/execution/algorithm/start_detached.h"
#include "di/execution/algorithm/then.h"
#include "di/execution/concepts/scheduler.h"
#include "di/execution/interface/schedule.h"
#include "di/execution/sequence/sequence_sender.h"
#include "di/function/invoke.h"
#include "di/function/tag_invoke.h"
#include "di/meta/util.h"
#include "di/platform/prelude.h"

namespace di::execution {
namespace execute_ns {
    struct Function {
        template<concepts::Scheduler Sched, concepts::MovableValue Fun, concepts::Allocator Alloc = DefaultAllocator>
        requires(concepts::Invocable<Fun>)
        auto operator()(Sched&& scheduler, Fun&& function, Alloc&& allocator = {}) const {
            if constexpr (concepts::TagInvocable<Function, Sched, Fun, Alloc>) {
                return function::tag_invoke(*this, util::forward<Sched>(scheduler), util::forward<Fun>(function),
                                            util::forward<Alloc>(allocator));
            } else {
                return start_detached(then(schedule(scheduler), util::forward<Fun>(function)),
                                      util::forward<Alloc>(allocator));
            }
        }
    };
}

/// @brief Executes a function on a scheduler.
///
/// @param scheduler The scheduler to execute the function on.
/// @param function The function which runs on the provided scheduler.
/// @param allocator The allocator used to allocate the operation state (optional).
///
/// @return Possibly an error indicating the operaiton state could not be allocated.
///
/// This function is like execution::start_detached() but does does not take a sender. Instead, it takes a function
/// which is invoked by the provided scheduler, which ensures all work is executed on the scheduler.
///
/// @see start_detached
/// @see then
/// @see schedule
constexpr inline auto execute = execute_ns::Function {};
}

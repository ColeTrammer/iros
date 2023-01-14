#pragma once

#include <di/execution/concepts/scheduler.h>
#include <di/execution/concepts/sender_of.h>
#include <di/function/tag_invoke.h>
#include <di/types/prelude.h>

namespace di::execution {
namespace async_write_ns {
    struct Function {
        template<concepts::Scheduler Sched>
        requires(concepts::TagInvocable<Function, Sched, int, Span<Byte const>, Optional<u64>>)
        concepts::SenderOf<NoEnv, size_t> auto operator()(Sched&& scheduler, int file_descriptor, Span<Byte const> buffer,
                                                          Optional<u64> offset = {}) const {
            return function::tag_invoke(*this, util::forward<Sched>(scheduler), file_descriptor, buffer, offset);
        }
    };
}

constexpr inline auto async_write = async_write_ns::Function {};
}
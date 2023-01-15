#pragma once

#include <di/container/path/prelude.h>
#include <di/execution/concepts/prelude.h>
#include <di/execution/meta/prelude.h>
#include <di/execution/types/prelude.h>

namespace di::execution {
namespace async_open_ns {
    struct Function {
        template<concepts::Scheduler Sched, typename... ExtraArgs>
        requires(concepts::TagInvocable<Function, Sched, ExtraArgs...>)
        concepts::Sender auto operator()(Sched&& sched, ExtraArgs&&... extra_args) const {
            return function::tag_invoke(*this, util::forward<Sched>(sched), util::forward<ExtraArgs>(extra_args)...);
        }
    };
}

constexpr inline auto async_open = async_open_ns::Function {};
}
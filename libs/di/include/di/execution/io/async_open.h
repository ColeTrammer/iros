#pragma once

#include "di/container/path/prelude.h"
#include "di/execution/concepts/prelude.h"
#include "di/execution/interface/run.h"
#include "di/execution/meta/prelude.h"
#include "di/execution/types/prelude.h"
#include "di/function/invoke.h"
#include "di/function/tag_invoke.h"

namespace di::execution {
namespace async_open_ns {
    struct Function {
        template<concepts::Scheduler Sched, typename... ExtraArgs>
        requires(concepts::TagInvocable<Function, Sched, ExtraArgs...>)
        auto operator()(Sched&& sched, ExtraArgs&&... extra_args) const {
            static_assert(
                concepts::AsyncResource<meta::InvokeResult<meta::TagInvokeResult<Function, Sched, ExtraArgs...>>>,
                "async_open() customizations must return a deferred di::AsyncResource instance.");
            return tag_invoke(*this, util::forward<Sched>(sched), util::forward<ExtraArgs>(extra_args)...);
        }
    };
}

constexpr inline auto async_open = async_open_ns::Function {};
}

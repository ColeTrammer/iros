#pragma once

#include <di/execution/concepts/queryable.h>
#include <di/execution/concepts/sender.h>
#include <di/execution/interface/get_env.h>
#include <di/execution/interface/schedule.h>
#include <di/execution/receiver/set_error.h>
#include <di/execution/receiver/set_stopped.h>
#include <di/execution/receiver/set_value.h>
#include <di/meta/compare.h>
#include <di/meta/core.h>
#include <di/meta/operations.h>

namespace di::execution {
template<concepts::OneOf<SetValue, SetError, SetStopped> CPO>
struct GetCompletionScheduler;
}

namespace di::concepts {
template<typename T>
concept Scheduler =
    CopyConstructible<meta::RemoveCVRef<T>> && EqualityComparable<meta::RemoveCVRef<T>> && Queryable<T> &&
    requires(T&& scheduler, execution::GetCompletionScheduler<execution::SetValue> const tag) {
        { execution::schedule(util::forward<T>(scheduler)) } -> Sender;
        {
            function::tag_invoke(tag, execution::get_env(execution::schedule(util::forward<T>(scheduler))))
        } -> SameAs<meta::RemoveCVRef<T>>;
    };
}

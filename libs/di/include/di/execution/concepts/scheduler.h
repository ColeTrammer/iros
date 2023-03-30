#pragma once

#include <di/concepts/copy_constructible.h>
#include <di/concepts/equality_comparable.h>
#include <di/concepts/one_of.h>
#include <di/execution/concepts/sender.h>
#include <di/execution/interface/schedule.h>
#include <di/execution/receiver/set_error.h>
#include <di/execution/receiver/set_stopped.h>
#include <di/execution/receiver/set_value.h>
#include <di/meta/remove_cvref.h>

namespace di::execution {
template<concepts::OneOf<SetValue, SetError, SetStopped> CPO>
struct GetCompletionScheduler;
}

namespace di::concepts {
template<typename T>
concept Scheduler = CopyConstructible<meta::RemoveCVRef<T>> && EqualityComparable<meta::RemoveCVRef<T>> &&
                    requires(T&& scheduler, execution::GetCompletionScheduler<execution::SetValue> const tag) {
                        { execution::schedule(util::forward<T>(scheduler)) } -> Sender;
                        {
                            function::tag_invoke(tag, execution::schedule(util::forward<T>(scheduler)))
                        } -> SameAs<meta::RemoveCVRef<T>>;
                    };
}

#pragma once

#include <di/concepts/default_constructible.h>
#include <di/execution/query/forwarding_scheduler_query.h>

namespace di::concepts {
template<typename Tag>
concept ForwardingSchedulerQuery = DefaultConstructible<Tag> && execution::forwarding_scheduler_query(Tag {});
}

#pragma once

#include <di/concepts/default_constructible.h>
#include <di/execution/query/forwarding_sender_query.h>

namespace di::concepts {
template<typename Tag>
concept ForwardingSenderQuery = DefaultConstructible<Tag> && execution::forwarding_sender_query(Tag {});
}
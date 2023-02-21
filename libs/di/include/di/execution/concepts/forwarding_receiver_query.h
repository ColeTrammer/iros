#pragma once

#include <di/concepts/default_constructible.h>
#include <di/execution/query/forwarding_receiver_query.h>

namespace di::concepts {
template<typename Tag>
concept ForwardingReceiverQuery = DefaultConstructible<Tag> && execution::forwarding_receiver_query(Tag {});
}

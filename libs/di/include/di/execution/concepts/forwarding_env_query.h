#pragma once

#include <di/concepts/default_constructible.h>
#include <di/execution/query/forwarding_env_query.h>

namespace di::concepts {
template<typename Tag>
concept ForwardingEnvQuery = DefaultConstructible<Tag> && execution::forwarding_env_query(Tag {});
}

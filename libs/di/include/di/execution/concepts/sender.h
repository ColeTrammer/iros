#pragma once

#include "di/execution/concepts/queryable.h"
#include "di/execution/interface/enable_sender.h"
#include "di/execution/interface/get_env.h"
#include "di/meta/core.h"
#include "di/meta/operations.h"

namespace di::concepts {
template<typename Send>
concept Sender = execution::enable_sender<meta::RemoveCVRef<Send>> && requires(meta::RemoveCVRef<Send> const& sender) {
    { execution::get_env(sender) } -> Queryable;
} && MoveConstructible<meta::RemoveCVRef<Send>> && ConstructibleFrom<meta::RemoveCVRef<Send>, Send>;
}

namespace di {
using concepts::Sender;
}

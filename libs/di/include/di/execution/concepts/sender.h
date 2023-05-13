#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/move_constructible.h>
#include <di/execution/concepts/queryable.h>
#include <di/execution/interface/enable_sender.h>
#include <di/meta/remove_cvref.h>

namespace di::concepts {
template<typename Send>
concept Sender = execution::enable_sender<meta::RemoveCVRef<Send>> && Queryable<Send> &&
                 MoveConstructible<meta::RemoveCVRef<Send>> && ConstructibleFrom<meta::RemoveCVRef<Send>, Send>;
}

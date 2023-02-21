#pragma once

#include <di/execution/interface/schedule.h>

namespace di::meta {
template<typename Sched>
using ScheduleResult = decltype(execution::schedule(util::declval<Sched>()));
}

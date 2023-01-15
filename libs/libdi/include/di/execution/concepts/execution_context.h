#pragma once

#include <di/execution/concepts/scheduler.h>

namespace di::concepts {
template<typename T>
concept ExecutionContext = requires(T& context) {
                               { context.get_scheduler() } -> Scheduler;
                               { context.finish() } -> LanguageVoid;
                               { context.run() } -> LanguageVoid;
                           };
}
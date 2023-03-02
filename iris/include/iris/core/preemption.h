#pragma once

#include <iris/core/task.h>

namespace iris {
class PreemptionDisabler {
public:
    PreemptionDisabler(Task& task) : m_task(&task) { m_task->disable_preemption(); }

    ~PreemptionDisabler() { m_task->enable_preemption(); }

private:
    Task* m_task { nullptr };
};

template<di::concepts::Invocable F>
decltype(auto) with_preemption_disabled(Task& task, F&& function) {
    auto guard = PreemptionDisabler { task };
    return di::invoke(di::forward<F>(function));
}
}

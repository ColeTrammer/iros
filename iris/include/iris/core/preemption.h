#pragma once

#include <iris/core/task.h>

namespace iris {
class PreemptionDisabler {
public:
    PreemptionDisabler();

    PreemptionDisabler(PreemptionDisabler&& other) : m_task(di::exchange(other.m_task, nullptr)) {}

    ~PreemptionDisabler();

private:
    Task* m_task { nullptr };
};

template<di::concepts::Invocable F>
decltype(auto) with_preemption_disabled(F&& function) {
    auto guard = PreemptionDisabler {};
    return di::invoke(di::forward<F>(function));
}
}

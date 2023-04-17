#pragma once

#include <di/function/prelude.h>
#include <di/util/prelude.h>

namespace iris {
class Task;

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

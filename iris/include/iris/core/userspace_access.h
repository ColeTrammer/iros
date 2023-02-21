#pragma once

#include <iris/core/global_state.h>

namespace iris {
// This is x86_64 specific.
class UserspaceAccessEnabler {
public:
    UserspaceAccessEnabler() {
        m_has_smap = !!(global_state().cpu_features & arch::CPUFeatures::Smap);
        if (m_has_smap) {
            asm volatile("stac" ::: "cc");
        }
    }

    ~UserspaceAccessEnabler() {
        if (m_has_smap) {
            asm volatile("clac" ::: "cc");
        }
    }

private:
    bool m_has_smap { false };
};

template<di::concepts::Invocable F>
decltype(auto) with_userspace_access(F&& function) {
    UserspaceAccessEnabler guard {};
    return di::invoke(di::forward<F>(function));
}
}
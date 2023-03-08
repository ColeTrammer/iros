#pragma once

#include <iris/core/global_state.h>

namespace iris {
class UserspaceAccessEnabler {
public:
    UserspaceAccessEnabler() {
        m_has_smap = !!(global_state().processor_info.features & ProcessorFeatures::Smap);
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
}

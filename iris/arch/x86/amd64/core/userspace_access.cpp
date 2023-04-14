#include <iris/core/global_state.h>
#include <iris/core/userspace_access.h>

namespace iris {
UserspaceAccessEnabler::UserspaceAccessEnabler() {
    m_has_smap = !!(global_state().processor_info.features & ProcessorFeatures::Smap);
    if (m_has_smap) {
        asm volatile("stac" ::: "cc");
    }
}

UserspaceAccessEnabler::~UserspaceAccessEnabler() {
    if (m_has_smap) {
        asm volatile("clac" ::: "cc");
    }
}

Expected<void> copy_to_user(di::Span<byte const> kernel_data, byte* userspace_ptr) {
    auto guard = UserspaceAccessEnabler {};
    di::copy(kernel_data, userspace_ptr);
    return {};
}

Expected<void> copy_from_user(di::Span<byte const> userspace_data, byte* kernel_ptr) {
    auto guard = UserspaceAccessEnabler {};
    di::copy(userspace_data, kernel_ptr);
    return {};
}
}

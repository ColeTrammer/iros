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

[[gnu::naked]] static Error do_userspace_copy(byte*, byte const*, usize) {
    asm volatile("mov %rdx, %rcx\n"

                 ".global __do_userspace_copy_instruction\n"
                 "__do_userspace_copy_instruction:\n"
                 "rep movsb\n"

                 "movl $0, %eax\n"

                 ".global __do_userspace_copy_return\n"
                 "__do_userspace_copy_return:\n"
                 "ret\n");
}

Expected<void> copy_to_user(di::Span<byte const> kernel_data, byte* userspace_ptr) {
    auto guard = UserspaceAccessEnabler {};
    auto result = do_userspace_copy(userspace_ptr, kernel_data.data(), kernel_data.size());
    if (result != Error::Success) {
        return di::Unexpected(result);
    }
    return {};
}

Expected<void> copy_from_user(di::Span<byte const> userspace_data, byte* kernel_ptr) {
    auto guard = UserspaceAccessEnabler {};
    auto result = do_userspace_copy(kernel_ptr, userspace_data.data(), userspace_data.size());
    if (result != Error::Success) {
        return di::Unexpected(result);
    }
    return {};
}
}

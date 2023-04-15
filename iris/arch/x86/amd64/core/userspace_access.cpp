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

static Expected<void> validate_userspace_address(uptr address) {
    // For now, the kernel takes the upper half of the address space.
    if (address & (1_u64 << 63)) {
        return di::Unexpected(Error::BadAddress);
    }
    return {};
}

static Expected<void> validate_canonical_address(uptr address) {
    // Normally, x86_64 requires the upper 16 bits be sign extended. This code will need to be updated if 5-level paging
    // is being used.
    auto upper_16_bits = (address >> 48) & 0xFFFF;
    auto bit_48 = (address >> 47) & 1;
    if ((bit_48 && upper_16_bits != 0xFFFF) || (!bit_48 && upper_16_bits != 0)) {
        return di::Unexpected(Error::BadAddress);
    }
    return {};
}

Expected<void> validate_user_region(mm::VirtualAddress userspace_address, usize count, usize size) {
    // FIXME: check for overflow here.
    auto size_bytes = size * count;

    auto begin = userspace_address.raw_value();
    auto end = userspace_address.raw_value() + size_bytes;

    // This prevents the region from wrapping around.
    if (end < begin) {
        return di::Unexpected(Error::BadAddress);
    }

    // Validate that the addresses are in the userspace.
    TRY(validate_userspace_address(begin));
    TRY(validate_userspace_address(end));

    // Validate that the addresses are canonical.
    TRY(validate_canonical_address(begin));
    TRY(validate_canonical_address(end));

    return {};
}
}

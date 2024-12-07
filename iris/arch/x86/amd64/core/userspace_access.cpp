#include "iris/core/userspace_access.h"

#include "iris/core/global_state.h"

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

[[gnu::naked]] static auto do_userspace_copy(byte*, byte const*, usize) -> Error {
    asm volatile("mov %rdx, %rcx\n"

                 ".global __do_userspace_copy_instruction\n"
                 "__do_userspace_copy_instruction:\n"
                 "rep movsb\n"

                 "movl $0, %eax\n"

                 ".global __do_userspace_copy_return\n"
                 "__do_userspace_copy_return:\n"
                 "ret\n");
}

auto copy_to_user(di::Span<byte const> kernel_data, byte* userspace_ptr) -> Expected<void> {
    auto guard = UserspaceAccessEnabler {};
    auto result = do_userspace_copy(userspace_ptr, kernel_data.data(), kernel_data.size());
    if (result != Error::Success) {
        return di::Unexpected(result);
    }
    return {};
}

auto copy_from_user(di::Span<byte const> userspace_data, byte* kernel_ptr) -> Expected<void> {
    auto guard = UserspaceAccessEnabler {};
    auto result = do_userspace_copy(kernel_ptr, userspace_data.data(), userspace_data.size());
    if (result != Error::Success) {
        return di::Unexpected(result);
    }
    return {};
}

static auto validate_userspace_address(uptr address) -> Expected<void> {
    // For now, the kernel takes the upper half of the address space.
    if (address & (1_u64 << 63)) {
        return di::Unexpected(Error::BadAddress);
    }
    return {};
}

static auto validate_canonical_address(uptr address) -> Expected<void> {
    // Normally, x86_64 requires the upper 16 bits be sign extended. This code will need to be updated if 5-level paging
    // is being used.
    auto upper_16_bits = (address >> 48) & 0xFFFF;
    auto bit_48 = (address >> 47) & 1;
    if ((bit_48 && upper_16_bits != 0xFFFF) || (!bit_48 && upper_16_bits != 0)) {
        return di::Unexpected(Error::BadAddress);
    }
    return {};
}

auto validate_user_region(mm::VirtualAddress userspace_address, usize count, usize size) -> Expected<void> {
    auto size_bytes = di::Checked(size) * count;
    if (!size_bytes.valid()) {
        return di::Unexpected(Error::BadAddress);
    }

    auto begin = userspace_address.raw_value();
    auto end = userspace_address.raw_value() + *size_bytes.value();

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

#pragma once

#include <iris/mm/virtual_address.h>

extern "C" {
extern void __do_userspace_copy_instruction();
extern void __do_userspace_copy_return();
}

namespace iris {
static inline mm::VirtualAddress kernel_userspace_copy_instruction((u64) &__do_userspace_copy_instruction);
static inline mm::VirtualAddress kernel_userspace_copy_return((u64) &__do_userspace_copy_return);

class UserspaceAccessEnabler {
public:
    UserspaceAccessEnabler();

    ~UserspaceAccessEnabler();

private:
    bool m_has_smap { false };
};
}

#ifndef _KERNEL_HAL_X86_64_PROCESSOR_H
#define _KERNEL_HAL_X86_64_PROCESSOR_H 1

#include <stdint.h>

struct arch_processor {
    uint8_t local_apic_id;
};

#endif /* _KERNEL_HAL_X86_64_PROCESSOR_H */

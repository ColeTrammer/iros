#ifndef _KERNEL_HAL_X86_64_HAL_H
#define _KERNEL_HAL_X86_64_HAL_H

#define INTERRUPTS_ENABLED_FLAG (1UL << 9UL)

bool cpu_supports_rdrand(void);

#endif /* _KERNEL_HAL_X86_64_HAL_H */

#ifndef _KERNEL_HAL_X86_HAL_H
#define _KERNEL_HAL_X86_HAL_H

#define INTERRUPTS_ENABLED_FLAG (1UL << 9UL)

bool cpu_supports_1gb_pages(void);
bool cpu_supports_rdrand(void);
bool found_acpi_tables(void);

#endif /* _KERNEL_HAL_X86_HAL_H */

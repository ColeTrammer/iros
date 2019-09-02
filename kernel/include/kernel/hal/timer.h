#ifndef _KERNEL_HAL_TIMER_H
#define _KERNEL_HAL_TIMER_H 1

#include <kernel/arch/x86_64/proc/process.h>

void register_callback(void (*callback)(struct process_state*), unsigned int ms);

#endif /* _KERNEL_HAL_TIMER_H */
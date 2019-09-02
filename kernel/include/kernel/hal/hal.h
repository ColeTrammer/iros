#ifndef _KERNEL_HAL_HAL_H
#define _KERNEL_HAL_HAL_H 1

void init_hal();
void init_drivers();

void enable_interrupts();
void disable_interrupts();

#endif /* _KERNEL_HAL_HAL_H */
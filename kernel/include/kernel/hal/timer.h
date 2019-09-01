#ifndef _KERNEL_HAL_TIMER_H
#define _KERNEL_HAL_TIMER_H 1

void register_callback(void (*callback)(void), unsigned int ms);

#endif /* _KERNEL_HAL_TIMER_H */
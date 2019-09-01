#ifndef _KERNEL_HAL_OUTPUT_H
#define _KERNEL_HAL_OUTPUT_H

#include <stdbool.h>
#include <stddef.h>

void init_output();

void dump_registers_to_screen();
bool kprint(const char *s, size_t len);

#endif /* _KERNEL_HAL_OUTPUT_H */
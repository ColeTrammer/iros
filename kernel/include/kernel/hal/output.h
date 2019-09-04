#ifndef _KERNEL_HAL_OUTPUT_H
#define _KERNEL_HAL_OUTPUT_H

#include <stdbool.h>
#include <stddef.h>

enum output_method {
    OUTPUT_SCREEN,
    OUTPUT_SERIAL
};

void init_output();

void dump_registers_to_screen();
bool kprint(const char *s, size_t len);
bool screen_print(const char *s, size_t len);

int debug_log(const char *f, ...);

#endif /* _KERNEL_HAL_OUTPUT_H */
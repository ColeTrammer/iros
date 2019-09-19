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

int debug_log_internal(const char *func, const char *f, ...) __attribute__((format (printf, 2, 3)));
void debug_log_assertion(const char *msg, const char *file, int line, const char *func);

#define debug_log(msg, ...) debug_log_internal(__func__, msg __VA_OPT__(,) __VA_ARGS__)

#endif /* _KERNEL_HAL_OUTPUT_H */
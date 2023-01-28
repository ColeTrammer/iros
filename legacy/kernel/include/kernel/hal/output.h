#ifndef _KERNEL_HAL_OUTPUT_H
#define _KERNEL_HAL_OUTPUT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

void dump_registers_to_screen();
bool kprint(const char *s, size_t len);

int vdebug_log_internal(const char *func, const char *f, va_list list) __attribute__((format(printf, 2, 0)));
int debug_log_internal(const char *func, const char *f, ...) __attribute__((format(printf, 2, 3)));
void __attribute__((__noreturn__)) debug_log_assertion(const char *msg, const char *file, int line, const char *func);

#ifndef vdebug_log
#define vdebug_log(msg, l) vdebug_log_internal(__func__, msg, l)
#endif /* vdebug_log */

#ifndef debug_log
#define debug_log(msg, ...) debug_log_internal(__func__, msg, ##__VA_ARGS__)
#endif /* debug_log */

#endif /* _KERNEL_HAL_OUTPUT_H */

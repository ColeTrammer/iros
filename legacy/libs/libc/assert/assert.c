#include <assert.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

void __attribute__((noreturn)) __assert_failed(const char *exp, const char *file, int line, const char *func) {
    fprintf(stderr, "Assertion failed: %s in %s at %s, line %d\n", exp, func, file, line);

    void *backtrace_buffer[100];
    int backtrace_size = backtrace(backtrace_buffer, sizeof(backtrace_buffer) / sizeof(backtrace_buffer[0]));
    dump_backtrace(backtrace_buffer, backtrace_size);

    abort();
}

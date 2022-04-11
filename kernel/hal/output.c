#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <kernel/hal/processor.h>
#include <kernel/proc/elf64.h>
#include <kernel/proc/task.h>
#include <kernel/util/spinlock.h>

spinlock_t debug_lock = SPINLOCK_INITIALIZER;

extern bool g_should_log;

int vdebug_log_internal(const char *func, const char *format, va_list parameters) {
    g_should_log = false;
    spin_lock_internal(&debug_lock, __func__, false);

    if (*format == '~') {
        int written = 0;
        written += printf("[Taskless Message]: ");
        written += vprintf(format + 1, parameters);

        spin_unlock(&debug_lock);
        g_should_log = true;
        va_end(parameters);
        return written;
    }

#ifndef KERNEL_NO_DEBUG_COLORS
    int written = 0;
    if (!bsp_enabled() || get_current_task() == NULL || get_current_task()->process->pid == 1) {
        written += printf("\033[35mKernel  \033[37m(\033[34m %d:%d \033[37m): ", 1, 1);
    } else {
        printf("\033[32m%s \033[37m(\033[34m %d:%d \033[37m): ", "Process", get_current_task()->process->pid, get_current_task()->tid);
    }
    written = printf("\033[36m%s\033[37m: ", func);
    written += vprintf(format, parameters);
#else
    int written = 0;
    if (!bsp_enabled() || get_current_task() == NULL || get_current_task()->process->pid == 1) {
        written += printf("Kernel  ( %d:%d ): ", 1, 1);
    } else {
        printf("%s ( %d:%d ): ", "Process", get_current_task()->process->pid, get_current_task()->tid);
    }
    written = printf("%s: ", func);
    written += vprintf(format, parameters);
#endif /* KERNEL_NO_DEBUG_COLORS */

    spin_unlock(&debug_lock);
    g_should_log = true;
    return written;
}

int debug_log_internal(const char *func, const char *format, ...) {
    va_list parameters;
    va_start(parameters, format);

    int written = vdebug_log_internal(func, format, parameters);

    va_end(parameters);
    return written;
}

static bool should_panic;

void debug_log_assertion(const char *msg, const char *file, int line, const char *func) {
    spin_lock_internal(&debug_lock, __func__, false);

#ifndef KERNEL_NO_DEBUG_COLORS
    printf("\n\033[31m");
#endif /* KERNEL_NO_DEBUG_COLORS */

    printf("( %d ): Assertion failed: %s in %s at %s, line %d\n", get_current_task()->process->pid, msg, func, file, line);

#ifndef KERNEL_NO_DEBUG_COLORS
    printf("\033[0m");
#endif /* KERNEL_NO_DEBUG_COLORS */

    if (should_panic) {
        abort();
    }

    // In case something else goes wrong, panic immediately without trying to dump a back trace
    should_panic = true;
    spin_unlock(&debug_lock);

    kernel_stack_trace((uintptr_t) &kernel_stack_trace, get_base_pointer());
    abort();
}

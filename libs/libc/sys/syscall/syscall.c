#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/syscall.h>

static long syscall0(int sc, va_list args) {
    (void) args;
    return __do_syscall(sc, 0, 0, 0, 0, 0, 0);
}

static long syscall1(int sc, va_list args) {
    unsigned long a1 = va_arg(args, unsigned long);
    return __do_syscall(sc, a1, 0, 0, 0, 0, 0);
}

static long syscall2(int sc, va_list args) {
    unsigned long a1 = va_arg(args, unsigned long);
    unsigned long a2 = va_arg(args, unsigned long);
    return __do_syscall(sc, a1, a2, 0, 0, 0, 0);
}

static long syscall3(int sc, va_list args) {
    unsigned long a1 = va_arg(args, unsigned long);
    unsigned long a2 = va_arg(args, unsigned long);
    unsigned long a3 = va_arg(args, unsigned long);
    return __do_syscall(sc, a1, a2, a3, 0, 0, 0);
}

static long syscall4(int sc, va_list args) {
    unsigned long a1 = va_arg(args, unsigned long);
    unsigned long a2 = va_arg(args, unsigned long);
    unsigned long a3 = va_arg(args, unsigned long);
    unsigned long a4 = va_arg(args, unsigned long);
    return __do_syscall(sc, a1, a2, a3, a4, 0, 0);
}

static long syscall5(int sc, va_list args) {
    unsigned long a1 = va_arg(args, unsigned long);
    unsigned long a2 = va_arg(args, unsigned long);
    unsigned long a3 = va_arg(args, unsigned long);
    unsigned long a4 = va_arg(args, unsigned long);
    unsigned long a5 = va_arg(args, unsigned long);
    return __do_syscall(sc, a1, a2, a3, a4, a5, 0);
}

static long syscall6(int sc, va_list args) {
    unsigned long a1 = va_arg(args, unsigned long);
    unsigned long a2 = va_arg(args, unsigned long);
    unsigned long a3 = va_arg(args, unsigned long);
    unsigned long a4 = va_arg(args, unsigned long);
    unsigned long a5 = va_arg(args, unsigned long);
    unsigned long a6 = va_arg(args, unsigned long);
    return __do_syscall(sc, a1, a2, a3, a4, a5, a6);
}

#undef __ENUMERATE_SYSCALL
#define __ENUMERATE_SYSCALL(s, a)       \
    case SC_##s:                        \
        ret = syscall##a(SC_##s, args); \
        break;

long syscall(enum sc_number sc, ...) {
    va_list args;
    va_start(args, sc);
    long ret;
    switch (sc) {
        ENUMERATE_SYSCALLS
        default:
            ret = -ENOSYS;
            break;
    }

    va_end(args);
    return ret;
}

#undef __ENUMERATE_SYSCALL
#define __ENUMERATE_SYSCALL(s, a) \
    case SC_##s:                  \
        return #s ": " #a " args";

char *syscall_to_string(enum sc_number sc) {
    switch (sc) {
        ENUMERATE_SYSCALLS
        default:
            return "Unknown system call";
    }
}
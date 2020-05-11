#include <sys/syscall.h>

#undef __ENUMERATE_SYSCALL
#define __ENUMERATE_SYSCALL(s, y, a) \
    case SC_##s:                     \
        return #y ": " #a " args";

char *syscall_to_string(enum sc_number sc) {
    switch (sc) {
        ENUMERATE_SYSCALLS
        default:
            return "Unknown system call";
    }
}

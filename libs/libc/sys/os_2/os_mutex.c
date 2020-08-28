#include <sys/os_2.h>
#include <sys/syscall.h>

int os_mutex(unsigned int *__protected, int op, int expected, int to_place, int to_wake, unsigned int *to_wait) {
    return syscall(SYS_os_mutex, __protected, op, expected, to_place, to_wake, to_wait);
}

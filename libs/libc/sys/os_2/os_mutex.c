#include <sys/os_2.h>
#include <sys/syscall.h>

int os_mutex(unsigned int *__protected, int op, int expected, int to_place, int to_wake, unsigned int *to_wait) {
    return syscall(SC_OS_MUTEX, __protected, op, expected, to_place, to_wake, to_wait);
}

#include <sys/syscall.h>
#include <unistd.h>

uid_t geteuid(void) {
    return (uid_t) syscall(SC_GETEUID);
}
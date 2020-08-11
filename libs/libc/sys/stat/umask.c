#include <errno.h>
#include <sys/stat.h>
#include <sys/syscall.h>

mode_t umask(mode_t mask) {
    return (mode_t) syscall(SYS_UMASK, mask);
}

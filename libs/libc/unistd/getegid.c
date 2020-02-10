#include <sys/syscall.h>
#include <unistd.h>

gid_t getegid(void) {
    return (gid_t) syscall(SC_GETEGID);
}
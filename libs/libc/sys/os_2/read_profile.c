#include <errno.h>
#include <sys/os_2.h>
#include <sys/syscall.h>

ssize_t read_profile(pid_t pid, void *buffer, size_t size) {
    int ret = (int) syscall(SC_READ_PROFILE, pid, buffer, size);
    __SYSCALL_TO_ERRNO(ret);
}

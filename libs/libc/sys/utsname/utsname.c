#include <errno.h>
#include <sys/syscall.h>
#include <sys/utsname.h>

int uname(struct utsname *buf) {
    int ret = (int) syscall(SYS_uname, buf);
    __SYSCALL_TO_ERRNO(ret);
}

#include <errno.h>
#include <sys/syscall.h>
#include <sys/utsname.h>

int uname(struct utsname *buf) {
    int ret = (int) syscall(SC_UNAME, buf);
    __SYSCALL_TO_ERRNO(ret);
}

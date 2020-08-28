#include <errno.h>
#include <sys/socket.h>
#include <sys/syscall.h>

int socketpair(int domain, int type, int protocol, int sv[2]) {
    int ret = (int) syscall(SYS_socketpair, domain, type, protocol, sv);
    __SYSCALL_TO_ERRNO(ret);
}

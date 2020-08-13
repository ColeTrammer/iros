#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/syscall.h>
#include <time.h>

int timer_create(clockid_t id, struct sigevent *__restrict sig, timer_t *__restrict timer) {
    int ret = (int) syscall(SYS_TIMER_CREATE, id, sig, timer);
    __SYSCALL_TO_ERRNO(ret);
}

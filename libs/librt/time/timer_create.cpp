#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <time.h>

extern "C" {
int timer_create(clockid_t id, struct sigevent *__restrict sig, timer_t *__restrict timer) {
    int ret = (int) syscall(SC_TIMER_CREATE, id, sig, timer);
    __SYSCALL_TO_ERRNO(ret);
}
}
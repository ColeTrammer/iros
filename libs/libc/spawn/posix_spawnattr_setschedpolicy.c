#include <errno.h>
#include <spawn.h>

int posix_spawnattr_setschedpolicy(posix_spawnattr_t *attr, int policy) {
    if (policy != SCHED_FIFO && policy != SCHED_RR && policy != SCHED_SPORADIC && policy != SCHED_OTHER) {
        return EINVAL;
    }

    attr->__sched_policy = policy;
    return 0;
}

#include <spawn.h>

int posix_spawnattr_getschedpolicy(const posix_spawnattr_t *__restrict attr, int *__restrict policy) {
    *policy = attr->__sched_policy;
    return 0;
}

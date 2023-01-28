#include <spawn.h>

int posix_spawnattr_getschedparam(const posix_spawnattr_t *__restrict attr, struct sched_param *__restrict param) {
    *param = attr->__sched_param;
    return 0;
}

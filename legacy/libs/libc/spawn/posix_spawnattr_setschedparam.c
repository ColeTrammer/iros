#include <spawn.h>

int posix_spawnattr_setschedparam(posix_spawnattr_t *__restrict attr, const struct sched_param *__restrict param) {
    attr->__sched_param = *param;
    return 0;
}

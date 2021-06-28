#include <spawn.h>

int posix_spawnattr_getpgroup(const posix_spawnattr_t *__restrict attr, pid_t *__restrict pidp) {
    *pidp = attr->__process_group;
    return 0;
}

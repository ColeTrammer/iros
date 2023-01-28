#include <errno.h>
#include <spawn.h>

int posix_spawnattr_setflags(posix_spawnattr_t *attr, short flags) {
    if (flags & ~(POSIX_SPAWN_RESETIDS | POSIX_SPAWN_SETPGROUP | POSIX_SPAWN_SETSCHEDPARAM | POSIX_SPAWN_SETSCHEDULER |
                  POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK)) {
        return EINVAL;
    }

    attr->__flags = flags;
    return 0;
}

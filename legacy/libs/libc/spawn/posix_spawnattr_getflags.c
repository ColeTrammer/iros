#include <spawn.h>

int posix_spawnattr_getflags(const posix_spawnattr_t *__restrict attr, short *__restrict flags) {
    *flags = attr->__flags;
    return 0;
}

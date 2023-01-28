#include <spawn.h>

int posix_spawnattr_setsigdefault(posix_spawnattr_t *__restrict attr, const sigset_t *__restrict setp) {
    attr->__signal_default = *setp;
    return 0;
}

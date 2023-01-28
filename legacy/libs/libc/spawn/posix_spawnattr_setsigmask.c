#include <spawn.h>

int posix_spawnattr_setsigmask(posix_spawnattr_t *__restrict attr, const sigset_t *__restrict setp) {
    attr->__signal_mask = *setp;
    return 0;
}

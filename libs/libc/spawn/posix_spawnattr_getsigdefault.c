#include <spawn.h>

int posix_spawnattr_getsigdefault(const posix_spawnattr_t *__restrict attr, sigset_t *__restrict setp) {
    *setp = attr->__signal_default;
    return 0;
}

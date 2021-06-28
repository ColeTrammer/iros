#include <errno.h>
#include <spawn.h>

int posix_spawnattr_setpgroup(posix_spawnattr_t *attr, pid_t pgroup) {
    if (pgroup < 0) {
        return EINVAL;
    }

    attr->__process_group = pgroup;
    return 0;
}

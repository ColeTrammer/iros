#include <spawn.h>

int posix_spawnattr_destroy(posix_spawnattr_t *attr) {
    *attr = (posix_spawnattr_t) { 0 };
    return 0;
}

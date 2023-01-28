#include <fcntl.h>
#include <unistd.h>

int chown(const char *pathname, uid_t owner, gid_t group) {
    return fchownat(AT_FDCWD, pathname, owner, group, 0);
}

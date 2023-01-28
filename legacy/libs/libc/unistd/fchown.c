#include <fcntl.h>
#include <unistd.h>

int fchown(int fd, uid_t owner, gid_t group) {
    return fchownat(fd, "", owner, group, AT_EMPTY_PATH);
}

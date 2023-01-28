#include <unistd.h>

// Currently all writes are synchronous, so fsync can be a no-op
int fsync(int fd) {
    (void) fd;
    return 0;
}

#include <errno.h>
#include <sys/resource.h>

int setrlimit(int resource, const struct rlimit *rlim) {
    (void) resource;
    (void) rlim;

    errno = EPERM;
    return -1;
}

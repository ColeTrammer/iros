#include <errno.h>
#include <stdio.h>
#include <sys/resource.h>

int getrlimit(int what, struct rlimit *res) {
    switch (what) {
        case RLIMIT_NOFILE:
            res->rlim_cur = res->rlim_max = FOPEN_MAX;
            return 0;
        default:
            errno = EINVAL;
            return -1;
    }
}

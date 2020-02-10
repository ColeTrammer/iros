#include <errno.h>
#include <limits.h>
#include <unistd.h>

int sysconf(int name) {
    switch (name) {
        case _SC_PAGE_SIZE:
            return PAGE_SIZE;
        default:
            errno = EINVAL;
            return 0;
    }
}

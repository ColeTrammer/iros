#include <errno.h>
#include <limits.h>
#include <unistd.h>

int sysconf(int name) {
    switch (name) {
        case _SC_PAGE_SIZE:
            return PAGE_SIZE;
        case _SC_CLK_TCK:
            return 100;
        default:
            errno = EINVAL;
            return 0;
    }
}

#include <errno.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

int gethostname(char *name, size_t len) {
    struct utsname u;
    if (uname(&u) < 0) {
        return -1;
    }

    size_t nodename_len = strlen(u.nodename);
    strncpy(name, u.nodename, len);
    if (nodename_len + 1 > len) {
        errno = ENAMETOOLONG;
        return -1;
    }

    return 0;
}

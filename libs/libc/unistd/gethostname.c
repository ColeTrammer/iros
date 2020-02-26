#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

int gethostname(char *name, size_t len) {
    struct utsname u;
    if (uname(&u) < 0) {
        return -1;
    }

    strncpy(name, u.nodename, len);
    return name;
}

#include <string.h>
#include <unistd.h>

#define GETLOGIN_BUF_SZ 64

static char buf[GETLOGIN_BUF_SZ];

char *getlogin(void) {
    getlogin_r(buf, GETLOGIN_BUF_SZ);
    return buf;
}

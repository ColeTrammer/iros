#include <string.h>
#include <unistd.h>

#define GETLOGIN_BUF_SZ 64

static char buf[GETLOGIN_BUF_SZ];

char *getlogin(void) {
    getlogin_r(buf, GETLOGIN_BUF_SZ);
    return buf;
}

int getlogin_r(char *buf, size_t bufsize) {
    (void) bufsize;
    strcpy(buf, "root");
    return 0;
}
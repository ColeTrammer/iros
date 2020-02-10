#include <string.h>
#include <unistd.h>

int getlogin_r(char *buf, size_t bufsize) {
    (void) bufsize;
    strcpy(buf, "root");
    return 0;
}
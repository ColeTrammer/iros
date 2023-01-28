#include <errno.h>
#include <stddef.h>
#include <syslog.h>

int main() {
    openlog(NULL, LOG_PERROR, LOG_USER);

    errno = EINTR;
    syslog(LOG_NOTICE, "Interesting error: %m");
    return 0;
}

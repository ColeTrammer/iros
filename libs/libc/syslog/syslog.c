#include <syslog.h>

void syslog(int prio, const char *format, ...) {
    (void) prio;
    (void) format;
}

#include <syslog.h>

void syslog(int prio, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    vsyslog(prio, format, ap);
    va_end(ap);
}

#include <syslog.h>

void openlog(const char *ident, int option, int facility) {
    (void) ident;
    (void) option;
    (void) facility;
}

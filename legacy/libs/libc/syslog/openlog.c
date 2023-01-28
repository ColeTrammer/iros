#include <syslog.h>

#include "syslog_internal.h"

void openlog(const char *ident, int option, int facility) {
    log_identity = ident;
    log_options = option;
    log_facility = facility;

    if (log_options & LOG_NOWAIT) {
        do_openlog();
    }
}

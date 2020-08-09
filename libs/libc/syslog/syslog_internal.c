#include <assert.h>
#include <stddef.h>
#include <syslog.h>

#include "syslog_internal.h"

int log_options = 0;
int log_facility = LOG_USER;
const char *log_identity = NULL;
FILE *log_file = NULL;

void do_openlog(void) {
    log_file = fopen("/dev/serial0", "w");
    assert(log_file);
}
